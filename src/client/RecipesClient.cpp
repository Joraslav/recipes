#include "RecipesClient.hpp"

#include "api/Json.hpp"
#include "api/Types.hpp"
#include "ClientConfig.hpp"
#include "Error.hpp"

#include <openssl/ssl.h>
#include <openssl/tls1.h>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/parser_fwd.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/cobalt/op.hpp>
#include <boost/cobalt/task.hpp>
#include <boost/system/system_error.hpp>

#include <chrono>
#include <cstdint>
#include <exception>
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = asio::ssl;
namespace cobalt = boost::cobalt;

using api::ParseProductResponses;
using api::ProductResponse;
using client::ClientConfig;
using client::Error;
using client::ErrorCode;

[[nodiscard]] Error MakeTransportError(const std::exception& exception) {
    return {ErrorCode::Transport, exception.what(), std::nullopt};
}

[[nodiscard]] Error MakeHttpError(http::status status) {
    return {ErrorCode::Http,
            std::format("HTTP request failed with status {}",
                        static_cast<unsigned>(status)),
            static_cast<uint16_t>(status)};
}

[[nodiscard]] http::request<http::empty_body> MakeRequest(
    const ClientConfig& config, http::verb method, std::string_view target) {
    http::request<http::empty_body> request{method, target, 11};
    request.keep_alive(config.keep_alive);
    request.set(http::field::host, config.host);
    request.set(http::field::accept, "application/json");
    return request;
}

template <typename Stream>
cobalt::task<std::expected<std::string, Error>> ReadResponse(
    Stream stream, std::shared_ptr<const ClientConfig> config) {
    try {
        beast::flat_buffer buffer;
        http::response_parser<http::string_body> parser;
        parser.body_limit(config->body_limit);
        parser.header_limit(config->header_limit);
        co_await http::async_read(stream, buffer, parser, cobalt::use_op);
        auto response = parser.release();
        if (response.result() != http::status::ok) {
            co_return std::unexpected(MakeHttpError(response.result()));
        }
        co_return response.body();
    } catch (const boost::system::system_error& exception) {
        co_return std::unexpected(MakeTransportError(exception));
    } catch (const std::exception& exception) {
        co_return std::unexpected(MakeTransportError(exception));
    }
}

template <typename Stream>
cobalt::task<std::expected<std::string, Error>> RequestTarget(
    Stream stream, std::shared_ptr<const ClientConfig> config,
    std::string_view target) {
    try {
        auto request = MakeRequest(*config, http::verb::get, target);
        co_await http::async_write(stream, request, cobalt::use_op);
        co_return co_await ReadResponse(std::move(stream), config);
    } catch (const std::exception& exception) {
        co_return std::unexpected(MakeTransportError(exception));
    }
}

[[nodiscard]] std::expected<void, Error> ConfigureSni(
    ssl::stream<beast::tcp_stream>& stream, const ClientConfig& config) {
    if (config.server_name.empty() ||
        SSL_set_tlsext_host_name(stream.native_handle(),
                                 config.server_name.c_str()) != 1) {
        return std::unexpected(Error{ErrorCode::Tls,
                                     "Failed to configure TLS server name",
                                     std::nullopt});
    }
    return {};
}

}  // namespace

namespace client {

RecipesClient::RecipesClient(asio::io_context& io_context, ClientConfig config)
    : config_(std::make_shared<const ClientConfig>(std::move(config))),
      io_context_(&io_context) {}

cobalt::task<std::expected<void, Error>> RecipesClient::Health() {
    try {
        asio::ip::tcp::resolver resolver{*io_context_};
        auto endpoints = co_await resolver.async_resolve(
            config_->host, std::to_string(config_->port), cobalt::use_op);
        if (config_->scheme == Scheme::Https) {
            ssl::context tls{ssl::context::tls_client};
            tls.set_verify_mode(config_->verify_peer ? ssl::verify_peer
                                                     : ssl::verify_none);
            if (config_->verify_peer && !config_->ca_file.empty()) {
                tls.load_verify_file(config_->ca_file.string());
            }
            ssl::stream<beast::tcp_stream> stream{
                beast::tcp_stream{*io_context_}, tls};
            if (const auto sni = ConfigureSni(stream, *config_); !sni) {
                co_return std::unexpected(sni.error());
            }
            stream.next_layer().expires_after(
                std::chrono::seconds{config_->timeout_seconds});
            co_await asio::async_connect(stream.next_layer().socket(),
                                         endpoints, cobalt::use_op);
            co_await stream.async_handshake(ssl::stream_base::client,
                                            cobalt::use_op);
            auto body =
                co_await RequestTarget(std::move(stream), config_, "/healthz");
            if (!body.has_value()) {
                co_return std::unexpected(body.error());
            }
            co_return {};
        }
        beast::tcp_stream stream{*io_context_};
        stream.expires_after(std::chrono::seconds{config_->timeout_seconds});
        co_await asio::async_connect(stream.socket(), endpoints,
                                     cobalt::use_op);
        auto body =
            co_await RequestTarget(std::move(stream), config_, "/healthz");
        if (!body.has_value()) {
            co_return std::unexpected(body.error());
        }
        co_return {};
    } catch (const std::exception& exception) {
        co_return std::unexpected(MakeTransportError(exception));
    }
}

cobalt::task<std::expected<std::vector<ProductResponse>, Error>>
RecipesClient::ListProducts() {
    try {
        asio::ip::tcp::resolver resolver{*io_context_};
        auto endpoints = co_await resolver.async_resolve(
            config_->host, std::to_string(config_->port), cobalt::use_op);
        if (config_->scheme == Scheme::Https) {
            ssl::context tls{ssl::context::tls_client};
            tls.set_verify_mode(config_->verify_peer ? ssl::verify_peer
                                                     : ssl::verify_none);
            if (config_->verify_peer && !config_->ca_file.empty()) {
                tls.load_verify_file(config_->ca_file.string());
            }
            ssl::stream<beast::tcp_stream> stream{
                beast::tcp_stream{*io_context_}, tls};
            if (const auto sni = ConfigureSni(stream, *config_); !sni) {
                co_return std::unexpected(sni.error());
            }
            stream.next_layer().expires_after(
                std::chrono::seconds{config_->timeout_seconds});
            co_await asio::async_connect(stream.next_layer().socket(),
                                         endpoints, cobalt::use_op);
            co_await stream.async_handshake(ssl::stream_base::client,
                                            cobalt::use_op);
            auto body = co_await RequestTarget(std::move(stream), config_,
                                               "/v1/products");
            if (!body.has_value()) {
                co_return std::unexpected(body.error());
            }
            auto products = ParseProductResponses(*body);
            if (!products.has_value()) {
                co_return std::unexpected(
                    Error{ErrorCode::Json, products.error(), std::nullopt});
            }
            co_return products.value();
        }
        beast::tcp_stream stream{*io_context_};
        stream.expires_after(std::chrono::seconds{config_->timeout_seconds});
        co_await asio::async_connect(stream.socket(), endpoints,
                                     cobalt::use_op);
        auto body =
            co_await RequestTarget(std::move(stream), config_, "/v1/products");
        if (!body.has_value()) {
            co_return std::unexpected(body.error());
        }
        auto products = ParseProductResponses(*body);
        if (!products.has_value()) {
            co_return std::unexpected(
                Error{ErrorCode::Json, products.error(), std::nullopt});
        }
        co_return products.value();
    } catch (const std::exception& exception) {
        co_return std::unexpected(MakeTransportError(exception));
    }
}

}  // namespace client
