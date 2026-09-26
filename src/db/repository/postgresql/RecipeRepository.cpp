#include "RecipeRepository.hpp"

#include "Error.hpp"
#include "SQL/PostgreSQLStatements.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/Recipe.hpp"
#include "types/kitchen/RecipeIngredient.hpp"

#include <exception>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pqxx/params"
#include "pqxx/result"
#include "pqxx/row"
#include "pqxx/transaction"

using db::repo::Error;
using db::repo::ErrorCode;

using types::Amount;
using types::Id;
using types::Recipe;
using types::RecipeIngredient;
using types::Result;

using Params = pqxx::params;
using Transaction = pqxx::transaction<>;

using PreparedStatement = db::stmt::PostgreSQLStatements;

using namespace std::string_view_literals;

namespace {

[[nodiscard]] RecipeIngredient MakeRecipeIngredientFromRow(
    const pqxx::row_ref& row) {
    return RecipeIngredient{row["product_type_id"].as<Id>(),
                            row["required_amount_base"].as<Amount>()};
}

[[nodiscard]] Recipe MakeRecipeFromRow(
    const pqxx::row_ref& row, std::vector<RecipeIngredient> ingredients) {
    return Recipe{row["name"].as<std::string>(),
                  row["description"].as<std::string>(), std::move(ingredients),
                  row["id"].as<Id>()};
}

}  // namespace

namespace db::repo::postgresql {

Result<Id, Error> RecipeRepository::Insert(const Recipe& recipe) {
    if (recipe.GetName().empty()) {
        return std::unexpected(
            Error{ErrorCode::InvalidData, "Recipe name cannot be empty"sv});
    }

    try {
        Transaction txn{*connection_};
        const auto result =
            txn.exec(PreparedStatement::kInsertRecipe,
                     Params{recipe.GetName(), recipe.GetDescription()});
        const auto recipe_id = result[0]["id"].as<Id>();

        for (const RecipeIngredient& ingredient : recipe.GetIngredients()) {
            txn.exec(PreparedStatement::kInsertRecipeIngredient,
                     Params{recipe_id, ingredient.GetProductTypeId(),
                            ingredient.GetRequiredAmount()});
        }

        txn.commit();
        return recipe_id;

    } catch (const pqxx::unique_violation& e) {
        return std::unexpected(Error{ErrorCode::AlreadyExists, e.what()});
    } catch (const pqxx::foreign_key_violation& e) {
        return std::unexpected(Error{ErrorCode::ConstraintViolation, e.what()});
    } catch (const pqxx::check_violation& e) {
        return std::unexpected(Error{ErrorCode::InvalidData, e.what()});
    } catch (const pqxx::not_null_violation& e) {
        return std::unexpected(Error{ErrorCode::InvalidData, e.what()});
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<std::optional<Recipe>, Error> RecipeRepository::GetById(Id id) {
    if (id <= 0) {
        return std::unexpected(
            Error{ErrorCode::InvalidData, "Recipe ID must be positive"});
    }

    try {
        Transaction txn{*connection_};
        const auto result = txn.exec(
            PreparedStatement::kSelectRecipeByIdWithIngredients, Params{id});

        if (result.empty()) {
            txn.commit();
            return std::nullopt;
        }

        std::vector<RecipeIngredient> ingredients;
        ingredients.reserve(result.size());
        for (const auto& row : result) {
            if (row["product_type_id"].is_null()) {
                continue;
            }
            ingredients.emplace_back(MakeRecipeIngredientFromRow(row));
        }

        Recipe recipe =
            MakeRecipeFromRow(result.front(), std::move(ingredients));
        txn.commit();

        return recipe;
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<std::vector<Recipe>, Error> RecipeRepository::GetAll() {
    try {
        Transaction txn{*connection_};
        const auto result =
            txn.exec(PreparedStatement::kSelectAllRecipesWithIngredients);

        std::vector<Recipe> recipes;
        if (result.empty()) {
            txn.commit();
            return recipes;
        }
        recipes.reserve(result.size());

        std::optional<Id> current_recipe_id;
        std::string current_name;
        std::string current_description;
        std::vector<RecipeIngredient> ingredients;

        for (const auto& row : result) {
            const Id recipe_id = row["id"].as<Id>();

            if (!current_recipe_id.has_value() ||
                current_recipe_id.value() != recipe_id) {

                if (current_recipe_id.has_value()) {
                    recipes.emplace_back(current_name, current_description,
                                         std::move(ingredients),
                                         current_recipe_id.value());
                }

                current_recipe_id = recipe_id;
                current_name = row["name"].as<std::string>();
                current_description = row["description"].as<std::string>();

                ingredients.clear();
            }

            if (!row["product_type_id"].is_null()) {
                ingredients.emplace_back(MakeRecipeIngredientFromRow(row));
            }
        }

        recipes.emplace_back(current_name, current_description,
                             std::move(ingredients), current_recipe_id.value());

        txn.commit();

        return recipes;
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<std::vector<Recipe>, Error> RecipeRepository::GetCookable() {
    try {
        Transaction txn{*connection_};
        const auto result =
            txn.exec(PreparedStatement::kSelectCookableRecipesWithIngredients);

        std::vector<Recipe> recipes;

        if (result.empty()) {
            txn.commit();
            return recipes;
        }

        recipes.reserve(result.size());

        std::optional<Id> current_recipe_id;
        std::string current_name;
        std::string current_description;
        std::vector<RecipeIngredient> ingredients;

        for (const auto& row : result) {
            const Id recipe_id = row["id"].as<Id>();

            if (!current_recipe_id.has_value() ||
                *current_recipe_id != recipe_id) {

                if (current_recipe_id.has_value()) {
                    recipes.emplace_back(current_name, current_description,
                                         std::move(ingredients),
                                         *current_recipe_id);
                }

                current_recipe_id = recipe_id;
                current_name = row["name"].as<std::string>();
                current_description = row["description"].as<std::string>();

                ingredients.clear();
            }

            ingredients.emplace_back(MakeRecipeIngredientFromRow(row));
        }

        recipes.emplace_back(current_name, current_description,
                             std::move(ingredients), *current_recipe_id);

        txn.commit();

        return recipes;
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<bool, Error> RecipeRepository::IsCookable(Id recipe_id) {
    if (recipe_id <= 0) {
        return std::unexpected(
            Error{ErrorCode::InvalidData, "Recipe ID must be positive"});
    }

    try {
        Transaction txn{*connection_};
        const auto result = txn.exec(PreparedStatement::kCheckRecipeCookable,
                                     Params{recipe_id});

        if (result.empty()) {
            txn.commit();
            return std::unexpected(
                Error{ErrorCode::DatabaseError,
                      "Cookable state query returned no rows"});
        }

        const bool cookable = result.front()["cookable"].as<bool>();

        txn.commit();

        return cookable;
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<void, Error> RecipeRepository::Update(const Recipe& recipe) {
    if (!recipe.GetId().has_value() || *recipe.GetId() <= 0) {
        return std::unexpected(Error{ErrorCode::InvalidData,
                                     "Recipe ID must be set and positive"});
    }
    if (recipe.GetName().empty()) {
        return std::unexpected(
            Error{ErrorCode::InvalidData, "Recipe name cannot be empty"sv});
    }

    const Id recipe_id = recipe.GetId().value();

    try {
        Transaction txn{*connection_};
        const auto result = txn.exec(
            PreparedStatement::kUpdateRecipeById,
            Params{recipe.GetName(), recipe.GetDescription(), recipe_id});
        if (result.affected_rows() == 0) {
            txn.abort();
            return std::unexpected(
                Error{ErrorCode::NotFound, "Recipe not found"sv});
        }

        txn.exec(PreparedStatement::kDeleteRecipeIngredientsByRecipeId,
                 Params{recipe_id});

        for (const RecipeIngredient& ingredient : recipe.GetIngredients()) {
            txn.exec(PreparedStatement::kInsertRecipeIngredient,
                     Params{recipe_id, ingredient.GetProductTypeId(),
                            ingredient.GetRequiredAmount()});
        }
        txn.commit();

        return {};
    } catch (const pqxx::unique_violation& e) {
        return std::unexpected(Error{ErrorCode::AlreadyExists, e.what()});
    } catch (const pqxx::foreign_key_violation& e) {
        return std::unexpected(Error{ErrorCode::ConstraintViolation, e.what()});
    } catch (const pqxx::check_violation& e) {
        return std::unexpected(Error{ErrorCode::InvalidData, e.what()});
    } catch (const pqxx::not_null_violation& e) {
        return std::unexpected(Error{ErrorCode::InvalidData, e.what()});
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<void, Error> RecipeRepository::Delete(Id id) {
    if (id <= 0) {
        return std::unexpected(
            Error{ErrorCode::InvalidData, "Recipe ID must be positive"});
    }

    try {
        Transaction txn{*connection_};
        const auto result =
            txn.exec(PreparedStatement::kDeleteRecipeById, Params{id});
        if (result.affected_rows() == 0) {
            txn.abort();
            return std::unexpected(
                Error{ErrorCode::NotFound, "Recipe not found"sv});
        }
        txn.commit();
        return {};
    } catch (const pqxx::foreign_key_violation& e) {
        return std::unexpected(Error{ErrorCode::ConstraintViolation, e.what()});
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

}  // namespace db::repo::postgresql