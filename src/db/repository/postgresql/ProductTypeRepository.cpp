#include "ProductTypeRepository.hpp"

#include "Error.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/ProductType.hpp"

#include <optional>
#include <string_view>
#include <vector>

using db::repo::Error;
using types::Id;
using types::ProductType;
using types::Result;

namespace db::repo::postgresql {

Result<Id, Error> ProductTypeRepository::Insert(const ProductType& product) {}

Result<std::optional<ProductType>, Error> ProductTypeRepository::GetById(
    Id id) {}

Result<std::optional<ProductType>, Error> ProductTypeRepository::GetByName(
    std::string_view name) {}

Result<std::vector<ProductType>, Error> ProductTypeRepository::GetAll() {}

Result<void, Error> ProductTypeRepository::Update(const ProductType& product) {}

Result<void, Error> ProductTypeRepository::Delete(Id id) {}

}  // namespace db::repo::postgresql