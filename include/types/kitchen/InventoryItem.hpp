#pragma once

#include "Defines.hpp"

#include <optional>

namespace types {

/**
 * @brief Inventory item representing a particular product batch.
 */
class InventoryItem final {
 public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    explicit InventoryItem(Id product_type_id, Amount amount, Dates dates = {},
                           std::optional<Id> id = std::nullopt)
        : product_type_id_(product_type_id),
          amount_(amount),
          dates_(dates),
          id_(id) {}
    InventoryItem(const InventoryItem&) = default;
    InventoryItem& operator=(const InventoryItem&) = default;
    InventoryItem(InventoryItem&&) = default;
    InventoryItem& operator=(InventoryItem&&) = default;
    ~InventoryItem() = default;

    void SetId(Id id) noexcept { id_ = id; }
    void SetProductTypeId(Id product_type_id) noexcept {
        product_type_id_ = product_type_id;
    }
    void SetAmount(Amount amount) { amount_ = amount; }
    void SetManufactureDate(Date date) noexcept { dates_.manufacture = date; }
    void SetExpirationDate(Date date) noexcept { dates_.expiration = date; }

    void ClearManufactureDate() noexcept { dates_.manufacture.reset(); }
    void ClearExpirationDate() noexcept { dates_.expiration.reset(); }

    [[nodiscard]] std::optional<Id> GetId() const noexcept { return id_; }
    [[nodiscard]] Id GetProductTypeId() const noexcept {
        return product_type_id_;
    }
    [[nodiscard]] Amount GetAmount() const noexcept { return amount_; }
    [[nodiscard]] std::optional<Date> GetManufactureDate() const noexcept {
        return dates_.manufacture;
    }
    [[nodiscard]] std::optional<Date> GetExpirationDate() const noexcept {
        return dates_.expiration;
    }

 private:
    Id product_type_id_;
    Amount amount_;
    Dates dates_;
    std::optional<Id> id_;
};

}  // namespace types