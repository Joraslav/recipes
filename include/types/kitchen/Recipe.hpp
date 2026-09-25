#pragma once

#include "Defines.hpp"
#include "RecipeIngredient.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace types {

class Recipe final {
 public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    explicit Recipe(std::string_view name, std::string_view description,
                    std::vector<RecipeIngredient> ingredients,
                    std::optional<Id> id = std::nullopt)
        : id_(id),
          name_(name),
          description_(description),
          ingredients_(std::move(ingredients)) {}
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    explicit Recipe(std::string_view name, std::string_view description,
                    std::span<RecipeIngredient> ingredients,
                    std::optional<Id> id = std::nullopt)
        : id_(id),
          name_(name),
          description_(description),
          ingredients_(ingredients.begin(), ingredients.end()) {}
    Recipe(const Recipe&) = default;
    Recipe& operator=(const Recipe&) = default;
    Recipe(Recipe&&) = default;
    Recipe& operator=(Recipe&&) = default;
    ~Recipe() = default;

    void SetId(Id id) noexcept { id_ = id; }
    void SetName(std::string_view name) { name_ = name; }
    void SetDescription(std::string_view description) {
        description_ = description;
    }
    void SetIngredients(std::vector<RecipeIngredient> ingredients) {
        ingredients_ = std::move(ingredients);
    }
    void SetIngredients(std::span<RecipeIngredient> ingredients) {
        ingredients_.assign(ingredients.begin(), ingredients.end());
    }
    void AddIngredient(RecipeIngredient ingredient) {
        ingredients_.push_back(std::move(ingredient));
    }

    [[nodiscard]] std::optional<Id> GetId() const noexcept { return id_; }
    [[nodiscard]] const std::string& GetName() const noexcept { return name_; }
    [[nodiscard]] const std::string& GetDescription() const noexcept {
        return description_;
    }
    [[nodiscard]] const std::vector<RecipeIngredient>& GetIngredients()
        const noexcept {
        return ingredients_;
    }

 private:
    std::optional<Id> id_;
    std::string name_;
    std::string description_;
    std::vector<RecipeIngredient> ingredients_;
};

}  // namespace types