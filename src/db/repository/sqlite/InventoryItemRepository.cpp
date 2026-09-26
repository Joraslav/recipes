#include "InventoryItemRepository.hpp"

#include "Error.hpp"
#include "SQL/SQLiteStatements.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/InventoryItem.hpp"

#include <SQLiteCpp/Column.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <expected>
#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using db::repo::Error;
using db::repo::ErrorCode;

using types::Amount;
using types::Date;
using types::Dates;
using types::Id;
using types::InventoryItem;
using types::Result;

using Statement = SQLite::Statement;
using Transaction = SQLite::Transaction;
using PreparedStatement = db::stmt::SQLiteStatements;

using namespace std::string_view_literals;

namespace {

[[nodiscard]] std::string DateToString(Date date) {
    return std::format("{:%Y-%m-%d}", date);
}

[[nodiscard]] std::optional<Date> StringToDate(const SQLite::Column& column) {
    if (column.isNull()) {
        return std::nullopt;
    }

    const std::string str = column.getString();
    const std::string_view value{str};

    if (value.size() < 10) {
        return std::nullopt;
    }

    constexpr std::string_view pattern = "0000-00-00";

    const bool valid = std::ranges::all_of(
        std::ranges::views::iota(size_t{0}, pattern.size()), [&](size_t i) {
            const char c = value[i];
            const char p = pattern[i];
            return p == '0' ? (c >= '0' && c <= '9') : (c == p);
        });

    if (!valid) {
        return std::nullopt;
    }

    const auto to_int = [](auto&& digit_range) {
        return std::ranges::fold_left(
            digit_range | std::views::transform([](char c) { return c - '0'; }),
            0, [](int acc, int digit) { return (acc * 10) + digit; });
    };

    const int y = to_int(value | std::views::take(4));
    const int m = to_int(value | std::views::drop(5) | std::views::take(2));
    const int d = to_int(value | std::views::drop(8) | std::views::take(2));

    const std::chrono::year_month_day ymd{
        std::chrono::year{y}, std::chrono::month{static_cast<unsigned>(m)},
        std::chrono::day{static_cast<unsigned>(d)}};

    if (!ymd.ok()) {
        return std::nullopt;
    }

    return Date{ymd};
}

}  // namespace

namespace db::repo::sqlite {

Result<Id, Error> InventoryItemRepository::Insert(const InventoryItem& item) {
    if (item.GetAmount() < 0) {
        return std::unexpected(Error{ErrorCode::InvalidData,
                                     "Inventory amount cannot be negative"sv});
    }

    try {
        Statement statement{
            *database_, std::string(PreparedStatement::kInsertInventoryItem)};

        statement.bind(1, item.GetProductTypeId());
        statement.bind(2, item.GetAmount());
        if (const auto manufacture = item.GetManufactureDate();
            manufacture.has_value()) {
            statement.bind(3, DateToString(manufacture.value()));
        } else {
            statement.bind(3, nullptr);
        }
        if (const auto expiration = item.GetExpirationDate();
            expiration.has_value()) {
            statement.bind(4, DateToString(expiration.value()));
        } else {
            statement.bind(4, nullptr);
        }

        if (!statement.executeStep()) {
            return std::unexpected(Error{ErrorCode::DatabaseError,
                                         "Failed to insert inventory item"sv});
        }

        return statement.getColumn(0).getInt64();
    } catch (const SQLite::Exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<std::optional<InventoryItem>, Error> InventoryItemRepository::GetById(
    Id id) {
    try {
        Statement statement{
            *database_,
            std::string(PreparedStatement::kSelectInventoryItemById)};

        statement.bind(1, id);

        if (!statement.executeStep()) {
            return std::optional<InventoryItem>{std::nullopt};
        }

        const Id item_id = statement.getColumn(0).getInt64();
        const Id product_type_id = statement.getColumn(1).getInt64();

        const Amount amount = statement.getColumn(4).getInt64();

        Dates dates;

        dates.manufacture = StringToDate(statement.getColumn(5));

        dates.expiration = StringToDate(statement.getColumn(6));

        return InventoryItem{product_type_id, amount, dates, item_id};

    } catch (const SQLite::Exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<std::vector<InventoryItem>, Error> InventoryItemRepository::GetAll() {
    try {
        Statement statement{
            *database_, std::string(PreparedStatement::kSelectAllInventory)};

        std::vector<InventoryItem> items;

        while (statement.executeStep()) {
            const Id item_id = statement.getColumn(0).getInt64();
            const Id product_type_id = statement.getColumn(1).getInt64();
            const Amount amount = statement.getColumn(4).getInt64();

            Dates dates;
            dates.manufacture = StringToDate(statement.getColumn(5));
            dates.expiration = StringToDate(statement.getColumn(6));

            items.emplace_back(product_type_id, amount, dates, item_id);
        }
        return items;
    } catch (const SQLite::Exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<std::vector<InventoryItem>, Error>
InventoryItemRepository::GetByProductTypeId(Id product_type_id) {
    try {
        Statement statement{
            *database_,
            std::string(PreparedStatement::kSelectInventoryByProductTypeId)};

        statement.bind(1, product_type_id);

        std::vector<InventoryItem> items;

        while (statement.executeStep()) {
            const Id item_id = statement.getColumn(0).getInt64();
            const Id product_type_id = statement.getColumn(1).getInt64();
            const Amount amount = statement.getColumn(4).getInt64();

            Dates dates;
            dates.manufacture = StringToDate(statement.getColumn(5));
            dates.expiration = StringToDate(statement.getColumn(6));

            items.emplace_back(product_type_id, amount, dates, item_id);
        }
        return items;

    } catch (const SQLite::Exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<Amount, Error> InventoryItemRepository::GetAvailableAmount(
    Id product_type_id) {
    try {
        Statement statement{
            *database_,
            std::string(
                PreparedStatement::kSelectAvailableAmountByProductTypeId)};

        statement.bind(1, product_type_id);

        if (!statement.executeStep()) {
            return std::unexpected(Error{ErrorCode::DatabaseError,
                                         "Failed to get available amount"sv});
        }

        const Amount available_amount = statement.getColumn(0).getInt64();
        return available_amount;

    } catch (const SQLite::Exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<void, Error> InventoryItemRepository::Update(const InventoryItem& item) {
    const auto item_id = item.GetId();

    if (!item_id.has_value()) {
        return std::unexpected(
            Error{ErrorCode::InvalidData,
                  "Inventory item ID is required for update"sv});
    }
    if (item.GetAmount() < 0) {
        return std::unexpected(
            Error{ErrorCode::InvalidData,
                  "Inventory item amount cannot be negative"sv});
    }

    try {
        Statement statement{
            *database_,
            std::string(PreparedStatement::kUpdateInventoryItemById)};

        statement.bind(1, item.GetProductTypeId());
        statement.bind(2, item.GetAmount());
        if (const auto manufacture = item.GetManufactureDate();
            manufacture.has_value()) {
            statement.bind(3, DateToString(manufacture.value()));
        } else {
            statement.bind(3, nullptr);
        }
        if (const auto expiration = item.GetExpirationDate();
            expiration.has_value()) {
            statement.bind(4, DateToString(expiration.value()));
        } else {
            statement.bind(4, nullptr);
        }
        statement.bind(5, item_id.value());
        std::ignore = statement.exec();

        if (database_->getChanges() == 0) {
            return std::unexpected(
                Error{ErrorCode::NotFound, "Inventory item not found"sv});
        }

        return {};

    } catch (const SQLite::Exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<void, Error> InventoryItemRepository::Delete(Id id) {
    try {
        Statement statement{
            *database_,
            std::string(PreparedStatement::kDeleteInventoryItemById)};

        statement.bind(1, id);
        std::ignore = statement.exec();

        if (database_->getChanges() == 0) {
            return std::unexpected(
                Error{ErrorCode::NotFound, "Inventory item not found"sv});
        }
        return {};
    } catch (const SQLite::Exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Result<void, Error> InventoryItemRepository::Consume(Id product_type_id,
                                                     Amount amount) {
    if (product_type_id <= 0) {
        return std::unexpected(
            Error{ErrorCode::InvalidData, "Product type ID must be positive"});
    }
    if (amount <= 0) {
        return std::unexpected(
            Error{ErrorCode::InvalidData,
                  "Consumption amount must be greater than zero"sv});
    }

    try {
        Transaction transaction{*database_};
        Statement select{
            *database_,
            std::string(PreparedStatement::kSelectInventoryForConsumption)};

        select.bind(1, product_type_id);
        Amount remaining = amount;
        while (select.executeStep()) {
            const Id item_id = select.getColumn(0).getInt64();
            const Amount available = select.getColumn(1).getInt64();
            const auto consumed = std::min(available, remaining);
            const auto new_amount = available - consumed;

            Statement update{
                *database_,
                std::string(PreparedStatement::kUpdateInventoryAmount)};

            update.bind(1, new_amount);
            update.bind(2, item_id);
            std::ignore = update.exec();

            remaining -= consumed;
            if (remaining <= 0) {
                break;
            }
        }

        if (remaining > 0) {
            transaction.rollback();
            return std::unexpected(Error{ErrorCode::NotCookable,
                                         "Insufficient inventory amount"sv});
        }
        transaction.commit();
        return {};

    } catch (const SQLite::Exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

}  // namespace db::repo::sqlite