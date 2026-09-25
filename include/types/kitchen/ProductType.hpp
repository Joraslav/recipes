#pragma once

#include "types/kitchen/Defines.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace types {

/**
 * @brief Product type metadata
 *
 * Describes a product independently of a particular inventory item
 *
 */
class ProductType final {
 public:
    explicit ProductType(std::string_view name, Dimension dimension,
                         std::optional<Id> id = std::nullopt)
        : name_(std::string(name)), dimension_(dimension), id_(id) {}
    ProductType(const ProductType&) = default;
    ProductType& operator=(const ProductType&) = default;
    ProductType(ProductType&&) = default;
    ProductType& operator=(ProductType&&) = default;
    ~ProductType() = default;

    void SetId(Id id) noexcept { id_ = id; }
    void SetName(std::string_view name) noexcept { name_ = name; }
    void SetDimension(Dimension dimension) noexcept { dimension_ = dimension; }

    [[nodiscard]] std::optional<Id> GetId() const noexcept { return id_; }
    [[nodiscard]] const std::string& GetName() const noexcept { return name_; }
    [[nodiscard]] Dimension GetDimension() const noexcept { return dimension_; }

 private:
    std::string name_;
    Dimension dimension_;
    std::optional<Id> id_;
};

}  // namespace types