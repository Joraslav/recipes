#pragma once

#include "api/Types.hpp"
#include "ClientConfig.hpp"
#include "Error.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/cobalt/task.hpp>

#include <expected>
#include <memory>
#include <vector>

namespace client {

class RecipesClient final {
 public:
    RecipesClient(boost::asio::io_context& io_context, ClientConfig config);

    [[nodiscard]] boost::cobalt::task<std::expected<void, Error>> Health();

    [[nodiscard]] boost::cobalt::task<
        std::expected<std::vector<api::ProductResponse>, Error>>
    ListProducts();

 private:
    std::shared_ptr<const ClientConfig> config_;
    boost::asio::io_context* io_context_;
};

}  // namespace client