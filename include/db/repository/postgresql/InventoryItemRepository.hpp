#pragma once

#include "Error.hpp"
#include "IInventoryItemRepository.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/InventoryItem.hpp"

#include <optional>
#include <stdexcept>
#include <vector>

#include "pqxx/connection"

namespace db::repo::postgresql {

class InventoryItemRepository final : public repo::IInventoryItemRepository {
 public:
    using Connection = pqxx::connection;

    explicit InventoryItemRepository(Connection* connection)
        : connection_(connection) {
        if (connection_ == nullptr) {
            throw std::invalid_argument("Connection pointer cannot be null");
        }
    }

    [[nodiscard]]
    types::Result<types::Id, Error> Insert(
        const types::InventoryItem& item) override;

    [[nodiscard]]
    types::Result<std::optional<types::InventoryItem>, Error> GetById(
        types::Id id) override;
    [[nodiscard]]
    types::Result<std::vector<types::InventoryItem>, Error> GetAll() override;
    [[nodiscard]]
    types::Result<std::vector<types::InventoryItem>, Error> GetByProductTypeId(
        types::Id product_type_id) override;
    [[nodiscard]]
    types::Result<types::Amount, Error> GetAvailableAmount(
        types::Id product_type_id) override;

    [[nodiscard]]
    types::Result<void, Error> Update(
        const types::InventoryItem& item) override;

    [[nodiscard]]
    types::Result<void, Error> Delete(types::Id id) override;

    [[nodiscard]]
    types::Result<void, Error> Consume(types::Id product_type_id,
                                       types::Amount amount) override;

 private:
    Connection* connection_;
};

}  // namespace db::repo::postgresql