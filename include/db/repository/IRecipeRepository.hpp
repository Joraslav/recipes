#pragma once

#include "Error.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/Recipe.hpp"

#include <optional>
#include <vector>

namespace db::repo {

class IRecipeRepository {
 public:
    virtual ~IRecipeRepository() = default;

    [[nodiscard]]
    virtual types::Result<types::Id, Error> Insert(
        const types::Recipe& recipe) = 0;

    [[nodiscard]]
    virtual types::Result<std::optional<types::Recipe>, Error> GetById(
        types::Id id) = 0;
    [[nodiscard]]
    virtual types::Result<std::vector<types::Recipe>, Error> GetAll() = 0;
    [[nodiscard]]
    virtual types::Result<std::vector<types::Recipe>, Error> GetCookable() = 0;
    [[nodiscard]]
    virtual types::Result<bool, Error> IsCookable(types::Id recipe_id) = 0;

    [[nodiscard]]
    virtual types::Result<void, Error> Update(const types::Recipe& recipe) = 0;

    [[nodiscard]]
    virtual types::Result<void, Error> Delete(types::Id id) = 0;
};

}  // namespace db::repo