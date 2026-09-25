#include "RecipeRepository.hpp"

#include "Error.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/Recipe.hpp"

#include <optional>
#include <string_view>
#include <vector>

using db::repo::Error;
using db::repo::ErrorCode;
using types::Id;
using types::Recipe;
using types::Result;

namespace db::repo::postgresql {

Result<Id, Error> RecipeRepository::Insert(const Recipe& recipe) {}

Result<std::optional<Recipe>, Error> RecipeRepository::GetById(Id id) {}

Result<std::vector<Recipe>, Error> RecipeRepository::GetAll() {}

Result<std::vector<Recipe>, Error> RecipeRepository::GetCookable() {}

Result<bool, Error> RecipeRepository::IsCookable(Id recipe_id) {}

Result<void, Error> RecipeRepository::Update(const Recipe& recipe) {}

Result<void, Error> RecipeRepository::Delete(Id id) {}

}  // namespace db::repo::postgresql