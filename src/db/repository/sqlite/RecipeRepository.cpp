#include "RecipeRepository.hpp"

#include "SQLiteCpp/Exception.h"

#include "Error.hpp"
#include "SQL/SQLiteStatements.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/Recipe.hpp"
#include "types/kitchen/RecipeIngredient.hpp"

#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using db::repo::Error;
using db::repo::ErrorCode;

using types::Amount;
using types::Id;
using types::Recipe;
using types::RecipeIngredient;
using types::Result;

using PreparedStatement = db::stmt::SQLiteStatements;

using Transaction = SQLite::Transaction;
using Statement = SQLite::Statement;

using namespace std::string_view_literals;

namespace {

[[nodiscard]] Error MakeDatabaseError(const SQLite::Exception& exception) {
    const std::string_view message{exception.what()};

    if (message.contains("UNIQUE constraint failed"sv)) {
        return Error{ErrorCode::AlreadyExists, exception.what()};
    }

    if (message.contains("FOREIGN KEY constraint failed"sv)) {
        return Error{ErrorCode::ConstraintViolation, exception.what()};
    }

    if (message.contains("CHECK constraint failed"sv)) {
        return Error{ErrorCode::ConstraintViolation, exception.what()};
    }

    return Error{ErrorCode::DatabaseError, exception.what()};
}

void AddIngredientFromRow(Recipe& recipe, Statement& statement) {
    if (statement.getColumn(3).isNull()) {
        return;
    }

    const Id product_type_id = statement.getColumn(3).getInt64();
    const Amount required_amount = statement.getColumn(6).getInt64();
    recipe.AddIngredient(RecipeIngredient{product_type_id, required_amount});
}

}  // namespace

namespace db::repo::sqlite {

Result<Id, Error> RecipeRepository::Insert(const Recipe& recipe) {
    try {
        Transaction transaction{*database_};

        Statement recipe_statement{
            *database_, std::string(PreparedStatement::kInsertRecipe)};
        recipe_statement.bindNoCopy(1, recipe.GetName());
        recipe_statement.bindNoCopy(2, recipe.GetDescription());
        if (!recipe_statement.executeStep()) {
            return std::unexpected(
                Error{ErrorCode::DatabaseError, "Failed to insert recipe"sv});
        }
        const Id recipe_id = recipe_statement.getColumn(0).getInt64();

        Statement ingredient_statement{
            *database_,
            std::string(PreparedStatement::kInsertRecipeIngredient)};
        for (const RecipeIngredient& ingredient : recipe.GetIngredients()) {
            ingredient_statement.bind(1, recipe_id);
            ingredient_statement.bind(2, ingredient.GetProductTypeId());
            ingredient_statement.bind(3, ingredient.GetRequiredAmount());

            std::ignore = ingredient_statement.exec();
            ingredient_statement.reset();
            ingredient_statement.clearBindings();
        }
        transaction.commit();

        return recipe_id;

    } catch (const SQLite::Exception& e) {
        return std::unexpected(MakeDatabaseError(e));
    }
}

Result<std::optional<Recipe>, Error> RecipeRepository::GetById(Id id) {
    try {
        Statement statement{
            *database_,
            std::string(PreparedStatement::kSelectRecipeByIdWithIngredients)};
        statement.bind(1, id);

        if (!statement.executeStep()) {
            return std::optional<Recipe>{std::nullopt};
        }

        Recipe recipe{statement.getColumn(1).getString(),
                      statement.getColumn(2).getString(),
                      std::vector<RecipeIngredient>{},
                      statement.getColumn(0).getInt64()};

        AddIngredientFromRow(recipe, statement);
        while (statement.executeStep()) {
            AddIngredientFromRow(recipe, statement);
        }

        return recipe;

    } catch (const SQLite::Exception& e) {
        return std::unexpected(MakeDatabaseError(e));
    }
}

Result<std::vector<Recipe>, Error> RecipeRepository::GetAll() {
    try {
        Statement statement{
            *database_,
            std::string(PreparedStatement::kSelectAllRecipesWithIngredients)};
        std::vector<Recipe> recipes;
        std::optional<Id> current_recipe_id;

        while (statement.executeStep()) {
            const Id recipe_id = statement.getColumn(0).getInt64();
            if (!current_recipe_id.has_value() ||
                current_recipe_id.value() != recipe_id) {
                recipes.emplace_back(statement.getColumn(1).getString(),
                                     statement.getColumn(2).getString(),
                                     std::vector<RecipeIngredient>{},
                                     recipe_id);
                current_recipe_id = recipe_id;
            }
            AddIngredientFromRow(recipes.back(), statement);
        }
        return recipes;

    } catch (const SQLite::Exception& e) {
        return std::unexpected(MakeDatabaseError(e));
    }
}

Result<std::vector<Recipe>, Error> RecipeRepository::GetCookable() {
    try {
        Statement statement{
            *database_,
            std::string(
                PreparedStatement::kSelectCookableRecipesWithIngredients)};
        std::vector<Recipe> recipes;
        std::optional<Id> current_recipe_id;

        while (statement.executeStep()) {
            const Id recipe_id = statement.getColumn(0).getInt64();
            if (!current_recipe_id.has_value() ||
                current_recipe_id.value() != recipe_id) {
                recipes.emplace_back(statement.getColumn(1).getString(),
                                     statement.getColumn(2).getString(),
                                     std::vector<RecipeIngredient>{},
                                     recipe_id);
                current_recipe_id = recipe_id;
            }
            AddIngredientFromRow(recipes.back(), statement);
        }
        return recipes;

    } catch (const SQLite::Exception& e) {
        return std::unexpected(MakeDatabaseError(e));
    }
}

Result<bool, Error> RecipeRepository::IsCookable(Id recipe_id) {
    try {
        Statement statement{
            *database_, std::string(PreparedStatement::kCheckRecipeCookable)};
        statement.bind(1, recipe_id);

        if (!statement.executeStep()) {
            return std::unexpected(Error{ErrorCode::DatabaseError,
                                         "Failed to check recipe cookable"sv});
        }
        return statement.getColumn(0).getInt() != 0;

    } catch (const SQLite::Exception& e) {
        return std::unexpected(MakeDatabaseError(e));
    }
}

Result<void, Error> RecipeRepository::Update(const Recipe& recipe) {
    if (!recipe.GetId().has_value()) {
        return std::unexpected(
            Error{ErrorCode::InvalidData, "Recipe ID is not set"sv});
    }

    const Id recipe_id = recipe.GetId().value();

    try {
        Transaction transaction{*database_};
        Statement select{*database_,
                         std::string(PreparedStatement::kSelectRecipeById)};
        select.bind(1, recipe_id);
        if (!select.executeStep()) {
            return std::unexpected(
                Error{ErrorCode::NotFound, "Recipe not found"sv});
        }

        Statement update{*database_,
                         std::string(PreparedStatement::kUpdateRecipeById)};
        update.bindNoCopy(1, recipe.GetName());
        update.bindNoCopy(2, recipe.GetDescription());
        update.bind(3, recipe_id);
        std::ignore = update.exec();

        Statement delete_ingredients{
            *database_,
            std::string(PreparedStatement::kDeleteRecipeIngredientsByRecipeId)};
        delete_ingredients.bind(1, recipe_id);
        std::ignore = delete_ingredients.exec();

        Statement insert_ingredient{
            *database_,
            std::string(PreparedStatement::kInsertRecipeIngredient)};
        for (const RecipeIngredient& ingredient : recipe.GetIngredients()) {
            insert_ingredient.bind(1, recipe_id);
            insert_ingredient.bind(2, ingredient.GetProductTypeId());
            insert_ingredient.bind(3, ingredient.GetRequiredAmount());

            insert_ingredient.exec();
            insert_ingredient.reset();
            insert_ingredient.clearBindings();
        }
        transaction.commit();

        return {};

    } catch (const SQLite::Exception& e) {
        return std::unexpected(MakeDatabaseError(e));
    }
}

Result<void, Error> RecipeRepository::Delete(Id id) {
    try {
        Statement statement{*database_,
                            std::string(PreparedStatement::kDeleteRecipeById)};
        statement.bind(1, id);
        std::ignore = statement.exec();

        if (database_->getChanges() == 0) {
            return std::unexpected(
                Error{ErrorCode::NotFound, "Recipe not found"sv});
        }

        return {};

    } catch (const SQLite::Exception& e) {
        return std::unexpected(MakeDatabaseError(e));
    }
}

}  // namespace db::repo::sqlite