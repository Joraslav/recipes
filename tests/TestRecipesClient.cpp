#include "gtest/gtest.h"

#include "app/DatabaseExecutor.hpp"
#include "client/ClientConfig.hpp"
#include "client/Error.hpp"
#include "client/RecipesClient.hpp"
#include "config/ServerConfig.hpp"
#include "server/Server.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/cobalt/spawn.hpp>

#include <expected>
#include <future>
#include <string>
#include <thread>
#include <utility>

using app::DatabaseExecutor;
using client::ClientConfig;
using client::Error;
using client::RecipesClient;
using config::ServerConfig;

namespace {

class TestRecipesClient : public ::testing::Test {};

TEST_F(TestRecipesClient, Health_OverHttp_ReturnsSuccess) {
    boost::asio::io_context io_context;
    DatabaseExecutor database_executor{":memory:", 8};
    ServerConfig server_config;
    server_config.http = {.enabled = true, .address = "127.0.0.1", .port = 0};
    net::Server server{database_executor, server_config};
    ASSERT_TRUE(server.Start().has_value());

    ClientConfig client_config;
    client_config.port = server.HttpPort();
    RecipesClient client{io_context, client_config};
    std::promise<std::expected<void, Error>> completion;
    auto result = completion.get_future();
    boost::cobalt::spawn(io_context, client.Health(),
                         [&completion](std::exception_ptr exception,
                                       std::expected<void, Error> value) {
                             if (exception) {
                                 completion.set_exception(exception);
                                 return;
                             }
                             completion.set_value(std::move(value));
                         });
    std::jthread worker([&io_context] { io_context.run(); });

    EXPECT_TRUE(result.get().has_value());
    server.Stop();
}

TEST_F(TestRecipesClient, Health_OverHttps_ReturnsSuccess) {
    boost::asio::io_context io_context;
    DatabaseExecutor database_executor{":memory:", 8};
    ServerConfig server_config;
    server_config.http.enabled = false;
    server_config.https = {.enabled = true, .address = "127.0.0.1", .port = 0};
    server_config.tls.certificate_path =
        std::string{RECIPES_TEST_SOURCE_DIR} + "/tests/data/certs/server.crt";
    server_config.tls.private_key_path =
        std::string{RECIPES_TEST_SOURCE_DIR} + "/tests/data/certs/server.key";
    net::Server server{database_executor, server_config};
    ASSERT_TRUE(server.Start().has_value());

    ClientConfig client_config;
    client_config.scheme = client::Scheme::Https;
    client_config.port = server.HttpsPort();
    client_config.verify_peer = false;
    RecipesClient client{io_context, client_config};
    std::promise<std::expected<void, client::Error>> completion;
    auto result = completion.get_future();
    boost::cobalt::spawn(io_context, client.Health(),
                         [&completion](std::exception_ptr exception,
                                       std::expected<void, Error> value) {
                             if (exception) {
                                 completion.set_exception(exception);
                                 return;
                             }
                             completion.set_value(std::move(value));
                         });
    std::jthread worker([&io_context] { io_context.run(); });

    EXPECT_TRUE(result.get().has_value());
    server.Stop();
}

}  // namespace
