#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>

namespace types {

/**
 * @brief Alias for type ID.
 */
using Id = int64_t;
/**
 * @brief Alias for date type representing system days.
 */
using Date = std::chrono::sys_days;
/**
 * @brief Alias for amount type in the base dimension.
 */
using Amount = int64_t;

/**
 * @brief Container for dates.
 * @details This structure holds optional manufacture and expiration dates for
 * an item.
 */
struct Dates final {
    std::optional<Date> manufacture;
    std::optional<Date> expiration;
};

/**
 * @brief Enumeration for measurement dimensions.
 * @details This enum represents various units of measurement for ingredients or
 * items in the kitchen.
 */
enum class Dimension : uint8_t { Gramm, Kilogramm, Milliliter, Liter, Piece };

/**
 * @brief Convert enum dimension to string constexpr
 * @param dimension Input enum param
 * @return Representation
 */
[[nodiscard]] constexpr std::string_view DimensionToString(
    Dimension dimension) noexcept {
    using namespace std::string_view_literals;
    switch (dimension) {
        case types::Dimension::Gramm:
            return "g"sv;
        case types::Dimension::Kilogramm:
            return "kg"sv;
        case types::Dimension::Milliliter:
            return "ml"sv;
        case types::Dimension::Liter:
            return "l"sv;
        case types::Dimension::Piece:
            return "pcs"sv;
        default:
            return "unknown"sv;
    }
}

}  // namespace types