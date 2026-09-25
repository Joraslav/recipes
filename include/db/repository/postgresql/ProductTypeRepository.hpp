#pragma once

#include "Error.hpp"
#include "IProductTypeRepository.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/ProductType.hpp"

#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "pqxx/connection"

namespace db::repo::postgresql {

class ProductTypeRepository final : public repo::IProductTypeRepository {
 public:
    using Connection = pqxx::connection;

    explicit ProductTypeRepository(Connection* connection)
        : connection_(connection) {
        if (connection_ == nullptr) {
            throw std::invalid_argument("Connection pointer cannot be null");
        }
    }

    [[nodiscard]]
    types::Result<types::Id, Error> Insert(
        const types::ProductType& product) override;

    [[nodiscard]]
    types::Result<std::optional<types::ProductType>, Error> GetById(
        types::Id id) override;
    [[nodiscard]]
    types::Result<std::optional<types::ProductType>, Error> GetByName(
        std::string_view name) override;
    [[nodiscard]]
    types::Result<std::vector<types::ProductType>, Error> GetAll() override;

    [[nodiscard]]
    types::Result<void, Error> Update(
        const types::ProductType& product) override;

    [[nodiscard]]
    types::Result<void, Error> Delete(types::Id id) override;

 private:
    Connection* connection_;
};

}  // namespace db::repo::postgresql