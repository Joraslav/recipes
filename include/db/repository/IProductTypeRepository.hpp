#pragma once

#include "Error.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/ProductType.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace db::repo {

class IProductTypeRepository {
 public:
    virtual ~IProductTypeRepository() = default;

    [[nodiscard]]
    virtual types::Result<types::Id, Error> Insert(
        const types::ProductType& product) = 0;

    [[nodiscard]]
    virtual types::Result<std::optional<types::ProductType>, Error> GetById(
        types::Id id) = 0;
    [[nodiscard]]
    virtual types::Result<std::optional<types::ProductType>, Error> GetByName(
        std::string_view name) = 0;
    [[nodiscard]]
    virtual types::Result<std::vector<types::ProductType>, Error> GetAll() = 0;

    [[nodiscard]]
    virtual types::Result<void, Error> Update(
        const types::ProductType& product) = 0;

    [[nodiscard]]
    virtual types::Result<void, Error> Delete(types::Id id) = 0;
};

}  // namespace db::repo