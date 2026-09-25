#include "InventoryItemRepository.hpp"

#include "Error.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/InventoryItem.hpp"

#include <optional>
#include <vector>

using db::repo::Error;
using types::Amount;
using types::Id;
using types::InventoryItem;
using types::Result;

namespace db::repo::postgresql {

Result<Id, Error> InventoryItemRepository::Insert(const InventoryItem& item) {}

Result<std::optional<InventoryItem>, Error> InventoryItemRepository::GetById(
    Id id) {}

Result<std::vector<InventoryItem>, Error> InventoryItemRepository::GetAll() {}

Result<std::vector<InventoryItem>, Error>
InventoryItemRepository::GetByProductTypeId(Id product_type_id) {}

Result<Amount, Error> InventoryItemRepository::GetAvailableAmount(
    Id product_type_id) {}

Result<void, Error> InventoryItemRepository::Update(const InventoryItem& item) {
}

Result<void, Error> InventoryItemRepository::Delete(Id id) {}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Result<void, Error> InventoryItemRepository::Consume(Id product_type_id,
                                                     Amount amount) {}

}  // namespace db::repo::postgresql