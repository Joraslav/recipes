#pragma once

#include "Defines.hpp"

namespace types {

class RecipeIngredient final {
 public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    explicit RecipeIngredient(Id product_type_id, Amount required_amount)
        : product_type_id_(product_type_id),
          required_amount_(required_amount) {}
    RecipeIngredient(const RecipeIngredient&) = default;
    RecipeIngredient& operator=(const RecipeIngredient&) = default;
    RecipeIngredient(RecipeIngredient&& other) noexcept
        : product_type_id_(other.product_type_id_),
          required_amount_(other.required_amount_) {
        other.product_type_id_ = Id{};
        other.required_amount_ = Amount{};
    }
    RecipeIngredient& operator=(RecipeIngredient&& other) noexcept {
        if (this != &other) {
            product_type_id_ = other.product_type_id_;
            required_amount_ = other.required_amount_;
            other.product_type_id_ = Id{};
            other.required_amount_ = Amount{};
        }
        return *this;
    }
    ~RecipeIngredient() = default;

    void SetRequiredAmount(Amount amount) noexcept {
        required_amount_ = amount;
    }

    [[nodiscard]] Id GetProductTypeId() const noexcept {
        return product_type_id_;
    }
    [[nodiscard]] Amount GetRequiredAmount() const noexcept {
        return required_amount_;
    }

 private:
    Id product_type_id_;
    Amount required_amount_;
};

}  // namespace types