#pragma once

#include <string_view>

namespace db::stmt {

/**
 * @brief SQL statements for SQLite database operations.
 *
 * Contains statements for database schema initialization, product types,
 * inventory, recipes, recipe ingredients, recipe availability checks,
 * and inventory consumption.
 */
struct SQLiteStatements final {
    /**
     * @brief Enables enforcement of foreign key constraints.
     * Must be executed for each SQLite database connection.
     */
    static constexpr std::string_view kEnableForeignKeys = R"sql(
PRAGMA foreign_keys = ON;
)sql";

    /**
     * @brief Creates the product_types table if it does not exist.
     * Stores product definitions and their base dimensions.
     */
    static constexpr std::string_view kCreateProductTypesTable = R"sql(
CREATE TABLE IF NOT EXISTS product_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    dimension INTEGER NOT NULL

    CHECK (dimension BETWEEN 0 AND 4)
);
)sql";

    /**
     * @brief Creates the inventory table if it does not exist.
     * Each row represents an individual inventory batch of a product type.
     */
    static constexpr std::string_view kCreateInventoryTable = R"sql(
CREATE TABLE IF NOT EXISTS inventory (
    id INTEGER PRIMARY KEY,
    product_type_id INTEGER NOT NULL,
    amount_base INTEGER NOT NULL CHECK (amount_base >= 0),
    manufacture_date DATE,
    expiration_date DATE,

    CHECK (
        manufacture_date IS NULL
        OR expiration_date IS NULL
        OR expiration_date >= manufacture_date
    ),

    FOREIGN KEY (product_type_id)
        REFERENCES product_types(id)
        ON DELETE CASCADE
);
)sql";

    /**
     * @brief Creates the recipes table if it does not exist.
     * Stores recipe names and optional Markdown descriptions.
     */
    static constexpr std::string_view kCreateRecipesTable = R"sql(
CREATE TABLE IF NOT EXISTS recipes (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    description TEXT
);
)sql";

    /**
     * @brief Creates the recipe_ingredients table if it does not exist.
     * Associates recipes with required product types and quantities.
     */
    static constexpr std::string_view kCreateRecipeIngredientsTable = R"sql(
CREATE TABLE IF NOT EXISTS recipe_ingredients (
    recipe_id INTEGER NOT NULL,
    product_type_id INTEGER NOT NULL,
    required_amount_base INTEGER NOT NULL CHECK (required_amount_base > 0),

    PRIMARY KEY (recipe_id, product_type_id),

    FOREIGN KEY (recipe_id)
        REFERENCES recipes(id)
        ON DELETE CASCADE,

    FOREIGN KEY (product_type_id)
        REFERENCES product_types(id)
        ON DELETE RESTRICT
);
)sql";

    /**
     * @brief Creates an index for searching recipes by product type.
     */
    static constexpr std::string_view kCreateRecipeIngredientsProductIndex =
        R"sql(
CREATE INDEX IF NOT EXISTS
    idx_recipe_ingredients_product_type
ON recipe_ingredients(product_type_id);
)sql";

    /**
     * @brief Creates an index for searching inventory by product type.
     */
    static constexpr std::string_view kCreateInventoryProductTypeIndex =
        R"sql(
CREATE INDEX IF NOT EXISTS
    idx_inventory_product_type
ON inventory(product_type_id);
)sql";

    /**
     * @brief Creates an index for inventory lookup ordered by expiration date.
     */
    static constexpr std::string_view kCreateInventoryExpirationIndex =
        R"sql(
CREATE INDEX IF NOT EXISTS
    idx_inventory_product_expiration
ON inventory(product_type_id, expiration_date);
)sql";

    /**
     * @brief Creates a product type and returns its generated ID.
     * @param[in] 1 Product name.
     * @param[in] 2 Base dimension.
     * @return ID of the created product type.
     */
    static constexpr std::string_view kInsertProductType = R"sql(
INSERT INTO product_types (
    name,
    dimension
)
VALUES (?, ?)
RETURNING id;
)sql";

    /**
     * @brief Selects all product types ordered by name.
     */
    static constexpr std::string_view kSelectAllProductTypes = R"sql(
SELECT
    id,
    name,
    dimension
FROM product_types
ORDER BY name;
)sql";

    /**
     * @brief Selects a product type by ID.
     * @param[in] 1 Product type ID.
     */
    static constexpr std::string_view kSelectProductTypeById = R"sql(
SELECT
    id,
    name,
    dimension
FROM product_types
WHERE id = ?;
)sql";

    /**
     * @brief Selects a product type by its name.
     * @param[in] 1 Product name.
     */
    static constexpr std::string_view kSelectProductTypeByName = R"sql(
SELECT
    id,
    name,
    dimension
FROM product_types
WHERE name = ?;
)sql";

    /**
     * @brief Updates a product type.
     * @param[in] 1 New product name.
     * @param[in] 2 New base dimension.
     * @param[in] 3 Product type ID.
     */
    static constexpr std::string_view kUpdateProductTypeById = R"sql(
UPDATE product_types
SET
    name = ?,
    dimension = ?
WHERE id = ?;
)sql";

    /**
     * @brief Deletes a product type by ID.
     * @param[in] 1 Product type ID.
     * @note Deletion is restricted when the product type is referenced
     *       by a recipe.
     */
    static constexpr std::string_view kDeleteProductTypeById = R"sql(
DELETE FROM product_types
WHERE id = ?;
)sql";

    /**
     * @brief Adds an inventory batch and returns its generated ID.
     * @param[in] 1 Product type ID.
     * @param[in] 2 Amount in the product type's base dimension.
     * @param[in] 3 Optional manufacture date.
     * @param[in] 4 Optional expiration date.
     * @return ID of the created inventory item.
     */
    static constexpr std::string_view kInsertInventoryItem = R"sql(
INSERT INTO inventory (
    product_type_id,
    amount_base,
    manufacture_date,
    expiration_date
)
VALUES (?, ?, ?, ?)
RETURNING id;
)sql";

    /**
     * @brief Selects an inventory item by ID.
     * Includes product type information.
     * @param[in] 1 Inventory item ID.
     */
    static constexpr std::string_view kSelectInventoryItemById = R"sql(
SELECT
    i.id,
    i.product_type_id,
    pt.name,
    pt.dimension,
    i.amount_base,
    i.manufacture_date,
    i.expiration_date
FROM inventory AS i
JOIN product_types AS pt
    ON pt.id = i.product_type_id
WHERE i.id = ?;
)sql";

    /**
     * @brief Selects all inventory items ordered by product and expiration
     * date.
     */
    static constexpr std::string_view kSelectAllInventory = R"sql(
SELECT
    i.id,
    i.product_type_id,
    pt.name,
    pt.dimension,
    i.amount_base,
    i.manufacture_date,
    i.expiration_date
FROM inventory AS i
JOIN product_types AS pt
    ON pt.id = i.product_type_id
ORDER BY
    pt.name,
    i.expiration_date,
    i.id;
)sql";

    /**
     * @brief Selects all inventory batches belonging to a product type.
     * @param[in] 1 Product type ID.
     */
    static constexpr std::string_view kSelectInventoryByProductTypeId = R"sql(
SELECT
    i.id,
    i.product_type_id,
    pt.name,
    pt.dimension,
    i.amount_base,
    i.manufacture_date,
    i.expiration_date
FROM inventory AS i
JOIN product_types AS pt
    ON pt.id = i.product_type_id
WHERE i.product_type_id = ?
ORDER BY
    i.expiration_date,
    i.id;
)sql";

    /**
     * @brief Updates an inventory item.
     * @param[in] 1 Product type ID.
     * @param[in] 2 New amount in the base dimension.
     * @param[in] 3 Optional manufacture date.
     * @param[in] 4 Optional expiration date.
     * @param[in] 5 Inventory item ID.
     */
    static constexpr std::string_view kUpdateInventoryItemById = R"sql(
UPDATE inventory
SET
    product_type_id = ?,
    amount_base = ?,
    manufacture_date = ?,
    expiration_date = ?
WHERE id = ?;
)sql";

    /**
     * @brief Deletes an inventory item by ID.
     * @param[in] 1 Inventory item ID.
     */
    static constexpr std::string_view kDeleteInventoryItemById = R"sql(
DELETE FROM inventory
WHERE id = ?;
)sql";

    /**
     * @brief Calculates the total available amount for every product type.
     */
    static constexpr std::string_view kSelectAvailableProductAmounts = R"sql(
SELECT
    pt.id,
    pt.name,
    pt.dimension,
    COALESCE(SUM(i.amount_base), 0) AS amount_base
FROM product_types AS pt
LEFT JOIN inventory AS i
    ON i.product_type_id = pt.id
GROUP BY
    pt.id,
    pt.name,
    pt.dimension
ORDER BY
    pt.name;
)sql";

    /**
     * @brief Calculates the total available amount of a product type.
     * @param[in] 1 Product type ID.
     */
    static constexpr std::string_view kSelectAvailableAmountByProductTypeId =
        R"sql(
SELECT
    COALESCE(SUM(amount_base), 0) AS amount_base
FROM inventory
WHERE product_type_id = ?;
)sql";

    /**
     * @brief Creates a recipe and returns its generated ID.
     * @param[in] 1 Recipe name.
     * @param[in] 2 Recipe description in Markdown format.
     * @return ID of the created recipe.
     */
    static constexpr std::string_view kInsertRecipe = R"sql(
INSERT INTO recipes (
    name,
    description
)
VALUES (?, ?)
RETURNING id;
)sql";

    /**
     * @brief Selects a recipe ID by name.
     * @param[in] 1 Recipe name.
     */
    static constexpr std::string_view kSelectRecipeIdByName = R"sql(
SELECT id
FROM recipes
WHERE name = ?;
)sql";

    /**
     * @brief Selects a recipe by ID.
     * @param[in] 1 Recipe ID.
     */
    static constexpr std::string_view kSelectRecipeById = R"sql(
SELECT
    id,
    name,
    description
FROM recipes
WHERE id = ?;
)sql";

    /**
     * @brief Selects all recipes ordered by name.
     */
    static constexpr std::string_view kSelectAllRecipes = R"sql(
SELECT
    id,
    name,
    description
FROM recipes
ORDER BY name;
)sql";

    /**
     * @brief Updates a recipe.
     * @param[in] 1 New recipe name.
     * @param[in] 2 New Markdown description.
     * @param[in] 3 Recipe ID.
     */
    static constexpr std::string_view kUpdateRecipeById = R"sql(
UPDATE recipes
SET
    name = ?,
    description = ?
WHERE id = ?;
)sql";

    /**
     * @brief Deletes a recipe by ID.
     * @param[in] 1 Recipe ID.
     * @note Recipe ingredients are deleted automatically by ON DELETE CASCADE.
     */
    static constexpr std::string_view kDeleteRecipeById = R"sql(
DELETE FROM recipes
WHERE id = ?;
)sql";

    /**
     * @brief Adds or updates an ingredient in a recipe.
     * @param[in] 1 Recipe ID.
     * @param[in] 2 Product type ID.
     * @param[in] 3 Required amount in the base dimension.
     */
    static constexpr std::string_view kInsertRecipeIngredient = R"sql(
INSERT INTO recipe_ingredients (
    recipe_id,
    product_type_id,
    required_amount_base
)
VALUES (?, ?, ?)
ON CONFLICT(recipe_id, product_type_id)
DO UPDATE SET
    required_amount_base = excluded.required_amount_base;
)sql";

    /**
     * @brief Selects all ingredients of a recipe.
     * @param[in] 1 Recipe ID.
     */
    static constexpr std::string_view kSelectRecipeIngredientsByRecipeId =
        R"sql(
SELECT
    ri.product_type_id,
    pt.name,
    pt.dimension,
    ri.required_amount_base
FROM recipe_ingredients AS ri
JOIN product_types AS pt
    ON pt.id = ri.product_type_id
WHERE ri.recipe_id = ?
ORDER BY pt.name;
)sql";

    /**
     * @brief Deletes all ingredients belonging to a recipe.
     * @param[in] 1 Recipe ID.
     * @note Intended for use when replacing the complete ingredient list.
     */
    static constexpr std::string_view kDeleteRecipeIngredientsByRecipeId =
        R"sql(
DELETE FROM recipe_ingredients
WHERE recipe_id = ?;
)sql";

    /**
     * @brief Selects a recipe together with all its ingredients.
     * @param[in] 1 Recipe ID.
     */
    static constexpr std::string_view kSelectRecipeByIdWithIngredients =
        R"sql(
SELECT
    r.id,
    r.name,
    r.description,
    ri.product_type_id,
    pt.name AS product_name,
    pt.dimension,
    ri.required_amount_base
FROM recipes AS r
LEFT JOIN recipe_ingredients AS ri
    ON ri.recipe_id = r.id
LEFT JOIN product_types AS pt
    ON pt.id = ri.product_type_id
WHERE r.id = ?
ORDER BY
    pt.name;
)sql";

    /**
     * @brief Selects all recipes together with their ingredients.
     */
    static constexpr std::string_view kSelectAllRecipesWithIngredients =
        R"sql(
SELECT
    r.id,
    r.name,
    r.description,
    ri.product_type_id,
    pt.name AS product_name,
    pt.dimension,
    ri.required_amount_base
FROM recipes AS r
LEFT JOIN recipe_ingredients AS ri
    ON ri.recipe_id = r.id
LEFT JOIN product_types AS pt
    ON pt.id = ri.product_type_id
ORDER BY
    r.name,
    pt.name;
)sql";

    /**
     * @brief Selects recipes that can currently be prepared.
     * A recipe is considered cookable when the total inventory amount
     * is sufficient for every required product type.
     */
    static constexpr std::string_view kSelectCookableRecipes =
        R"sql(
SELECT
    r.id,
    r.name,
    r.description
FROM recipes AS r
WHERE NOT EXISTS (
    SELECT 1
    FROM recipe_ingredients AS ri
    WHERE ri.recipe_id = r.id
      AND ri.required_amount_base > (
          SELECT COALESCE(SUM(i.amount_base), 0)
          FROM inventory AS i
          WHERE i.product_type_id = ri.product_type_id
      )
)
ORDER BY r.name;
)sql";

    /**
     * @brief Selects cookable recipes together with their ingredients.
     */
    static constexpr std::string_view kSelectCookableRecipesWithIngredients =
        R"sql(
SELECT
    r.id,
    r.name,
    r.description,
    ri.product_type_id,
    pt.name AS product_name,
    pt.dimension,
    ri.required_amount_base
FROM recipes AS r
JOIN recipe_ingredients AS ri
    ON ri.recipe_id = r.id
JOIN product_types AS pt
    ON pt.id = ri.product_type_id
WHERE NOT EXISTS (
    SELECT 1
    FROM recipe_ingredients AS ri2
    WHERE ri2.recipe_id = r.id
      AND ri2.required_amount_base > (
          SELECT COALESCE(SUM(i.amount_base), 0)
          FROM inventory AS i
          WHERE i.product_type_id = ri2.product_type_id
      )
)
ORDER BY
    r.name,
    pt.name;
)sql";

    /**
     * @brief Checks whether a recipe can currently be prepared.
     * @param[in] 1 Recipe ID.
     * @return A boolean column named @c cookable.
     */
    static constexpr std::string_view kCheckRecipeCookable = R"sql(
SELECT NOT EXISTS (
    SELECT 1
    FROM recipe_ingredients AS ri
    WHERE ri.recipe_id = ?
      AND ri.required_amount_base > (
          SELECT COALESCE(SUM(i.amount_base), 0)
          FROM inventory AS i
          WHERE i.product_type_id = ri.product_type_id
      )
) AS cookable;
)sql";

    /**
     * @brief Selects inventory batches available for consumption.
     * Results are ordered according to FEFO (First Expired, First Out).
     * @param[in] 1 Product type ID.
     */
    static constexpr std::string_view kSelectInventoryForConsumption =
        R"sql(
SELECT
    id,
    amount_base,
    manufacture_date,
    expiration_date
FROM inventory
WHERE product_type_id = ?
  AND amount_base > 0
ORDER BY
    expiration_date IS NULL,
    expiration_date,
    id;
)sql";

    /**
     * @brief Updates the amount remaining in an inventory batch.
     * @param[in] 1 New amount in the base dimension.
     * @param[in] 2 Inventory item ID.
     */
    static constexpr std::string_view kUpdateInventoryAmount = R"sql(
UPDATE inventory
SET amount_base = ?
WHERE id = ?;
)sql";

    /**
     * @brief Deletes an inventory batch with zero remaining quantity.
     * @param[in] 1 Inventory item ID.
     */
    static constexpr std::string_view kDeleteEmptyInventoryItem = R"sql(
DELETE FROM inventory
WHERE id = ?
  AND amount_base = 0;
)sql";
};

}  // namespace db::stmt
