#pragma once

#include "Error.hpp"
#include "IRecipeRepository.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/Recipe.hpp"

#include <optional>
#include <stdexcept>
#include <vector>

#include "pqxx/connection"

namespace db::repo::postgresql {

class RecipeRepository final : public repo::IRecipeRepository {
 public:
    using Connection = pqxx::connection;

    explicit RecipeRepository(Connection* connection)
        : connection_(connection) {
        if (connection_ == nullptr) {
            throw std::invalid_argument("Connection pointer cannot be null");
        }
    }

    [[nodiscard]]
    types::Result<types::Id, Error> Insert(
        const types::Recipe& recipe) override;

    [[nodiscard]]
    types::Result<std::optional<types::Recipe>, Error> GetById(
        types::Id id) override;
    [[nodiscard]]
    types::Result<std::vector<types::Recipe>, Error> GetAll() override;
    [[nodiscard]]
    types::Result<std::vector<types::Recipe>, Error> GetCookable() override;
    [[nodiscard]]
    types::Result<bool, Error> IsCookable(types::Id recipe_id) override;

    [[nodiscard]]
    types::Result<void, Error> Update(const types::Recipe& recipe) override;

    [[nodiscard]]
    types::Result<void, Error> Delete(types::Id id) override;

 private:
    Connection* connection_;
};

}  // namespace db::repo::postgresql