#pragma once

#include "Error.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/InventoryItem.hpp"

#include <optional>
#include <vector>

namespace db::repo {

class IInventoryItemRepository {
 public:
    virtual ~IInventoryItemRepository() = default;

    [[nodiscard]]
    virtual types::Result<types::Id, Error> Insert(
        const types::InventoryItem& item) = 0;

    [[nodiscard]]
    virtual types::Result<std::optional<types::InventoryItem>, Error> GetById(
        types::Id id) = 0;
    [[nodiscard]]
    virtual types::Result<std::vector<types::InventoryItem>, Error>
    GetAll() = 0;
    [[nodiscard]]
    virtual types::Result<std::vector<types::InventoryItem>, Error>
    GetByProductTypeId(types::Id product_type_id) = 0;
    [[nodiscard]]
    virtual types::Result<types::Amount, Error> GetAvailableAmount(
        types::Id product_type_id) = 0;

    [[nodiscard]]
    virtual types::Result<void, Error> Update(
        const types::InventoryItem& item) = 0;

    [[nodiscard]]
    virtual types::Result<void, Error> Delete(types::Id id) = 0;

    [[nodiscard]]
    virtual types::Result<void, Error> Consume(types::Id product_type_id,
                                               types::Amount amount) = 0;
};

}  // namespace db::repo