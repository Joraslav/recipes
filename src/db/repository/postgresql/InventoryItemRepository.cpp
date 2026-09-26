#include "InventoryItemRepository.hpp"

#include "Error.hpp"
#include "SQL/PostgreSQLStatements.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/InventoryItem.hpp"

#include <algorithm>
#include <chrono>
#include <exception>
#include <expected>
#include <format>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "pqxx/result"
#include "pqxx/row"
#include "pqxx/transaction"

using db::repo::Error;
using db::repo::ErrorCode;

using types::Amount;
using types::Date;
using types::Dates;
using types::Id;
using types::InventoryItem;
using types::Result;

using Params = pqxx::params;
using Transaction = pqxx::transaction<>;

using PreparedStatement = db::stmt::PostgreSQLStatements;

using namespace std::string_view_literals;

namespace {

[[nodiscard]] std::string DateToString(Date date) {
    return std::format("{:%Y-%m-%d}", date);
}

[[nodiscard]] Date StringToDate(std::string_view str) {
    if (str.size() != 10 || str[4] != '-' || str[7] != '-') {
        throw std::invalid_argument(
            "Invalid PostgreSQL date format: expected YYYY-MM-DD");
    }

    const bool digits_ok = std::ranges::all_of(
        str | std::views::enumerate | std::views::filter([](auto p) {
            auto [i, c] = p;
            return i != 4 && i != 7;
        }) | std::views::values,
        [](char c) { return c >= '0' && c <= '9'; });
    if (!digits_ok) {
        throw std::invalid_argument(
            "Invalid PostgreSQL date format: non-digit character");
    }

    auto parse_int = [](std::string_view s) -> int {
        int value = 0;
        for (char c : s) {
            value = (value * 10) + (c - '0');
        }
        return value;
    };

    const int y = parse_int(str.substr(0, 4));
    const int m = parse_int(str.substr(5, 2));
    const int d = parse_int(str.substr(8, 2));

    const std::chrono::year_month_day ymd{
        std::chrono::year{y}, std::chrono::month{static_cast<unsigned>(m)},
        std::chrono::day{static_cast<unsigned>(d)}};

    if (!ymd.ok()) {
        throw std::invalid_argument("Invalid PostgreSQL date value");
    }

    return Date{ymd};
}

[[nodiscard]] std::optional<Date> GetNullableDateFromField(
    const pqxx::field_ref& field) {
    if (field.is_null()) {
        return std::nullopt;
    }

    return StringToDate(field.c_str());
}

[[nodiscard]] InventoryItem MakeInventoryItemFromRow(const pqxx::row_ref& row) {
    const auto id = row["id"].as<Id>();
    const auto product_type_id = row["product_type_id"].as<Id>();
    const auto amount = row["amount_base"].as<Amount>();

    Dates dates{
        .manufacture = GetNullableDateFromField(row["manufacture_date"]),
        .expiration = GetNullableDateFromField(row["expiration_date"])};

    return InventoryItem{product_type_id, amount, dates, id};
}

}  // namespace

namespace db::repo::postgresql {

Result<Id, Error> InventoryItemRepository::Insert(const InventoryItem& item) {
    const Id product_type_id = item.GetProductTypeId();
    const Amount amount = item.GetAmount();
    if (product_type_id <= 0) {
        return std::unexpected(Error{ErrorCode::InvalidData,
                                     "Product type ID must be positive"sv});
    }
    if (amount < 0) {
        return std::unexpected(
            Error{ErrorCode::InvalidData, "Amount must be positive"sv});
    }

    try {
        Transaction txn{*connection_};
        Params params;
        params.append(product_type_id);
        params.append(amount);
        const auto manufacture_date = item.GetManufactureDate();
        const auto expiration_date = item.GetExpirationDate();
        if (manufacture_date.has_value()) {
            params.append(DateToString(manufacture_date.value()));
        } else {
            params.append(nullptr);
        }
        if (expiration_date.has_value()) {
            params.append(DateToString(expiration_date.value()));
        } else {
            params.append(nullptr);
        }

        const auto result =
            txn.exec(PreparedStatement::kInsertInventoryItem, params);
        txn.commit();

        return result[0]["id"].as<Id>();

    } catch (const pqxx::foreign_key_violation& e) {
        return std::unexpected(Error{ErrorCode::ConstraintViolation, e.what()});
    } catch (const pqxx::check_violation& e) {
        return std::unexpected(Error{ErrorCode::InvalidData, e.what()});
    } catch (const pqxx::not_null_violation& e) {
        return std::unexpected(Error{ErrorCode::InvalidData, e.what()});
    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<std::optional<InventoryItem>, Error> InventoryItemRepository::GetById(
    Id id) {
    if (id <= 0) {
        return std::unexpected(Error{ErrorCode::InvalidData,
                                     "Inventory item ID must be positive"});
    }

    try {
        Transaction txn{*connection_};
        const auto result =
            txn.exec(PreparedStatement::kSelectInventoryItemById, Params{id});
        txn.commit();

        if (result.empty()) {
            return std::nullopt;
        }

        return MakeInventoryItemFromRow(result[0]);

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<std::vector<InventoryItem>, Error> InventoryItemRepository::GetAll() {
    try {
        Transaction txn{*connection_};
        const auto result = txn.exec(PreparedStatement::kSelectAllInventory);
        txn.commit();

        std::vector<InventoryItem> items;
        items.reserve(result.size());
        for (const auto& row : result) {
            items.emplace_back(MakeInventoryItemFromRow(row));
        }

        return items;
    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<std::vector<InventoryItem>, Error>
InventoryItemRepository::GetByProductTypeId(Id product_type_id) {
    if (product_type_id <= 0) {
        return std::unexpected(Error{ErrorCode::InvalidData,
                                     "Product type ID must be positive"sv});
    }

    try {
        Transaction txn{*connection_};
        const auto result =
            txn.exec(PreparedStatement::kSelectInventoryByProductTypeId,
                     Params{product_type_id});
        txn.commit();

        std::vector<InventoryItem> items;
        items.reserve(result.size());
        for (const auto& row : result) {
            items.emplace_back(MakeInventoryItemFromRow(row));
        }

        return items;
    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<Amount, Error> InventoryItemRepository::GetAvailableAmount(
    Id product_type_id) {
    if (product_type_id <= 0) {
        return std::unexpected(Error{ErrorCode::InvalidData,
                                     "Product type ID must be positive"sv});
    }

    try {
        Transaction txn{*connection_};
        const auto result =
            txn.exec(PreparedStatement::kSelectAvailableAmountByProductTypeId,
                     Params{product_type_id});
        txn.commit();

        return result[0]["amount_base"].as<Amount>();

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<void, Error> InventoryItemRepository::Update(const InventoryItem& item) {
    const auto id = item.GetId();
    if (!id.has_value()) {
        return std::unexpected(
            Error{ErrorCode::InvalidData,
                  "Inventory item ID is required for update"sv});
    }
    if (item.GetProductTypeId() <= 0) {
        return std::unexpected(Error{ErrorCode::InvalidData,
                                     "Product type ID must be positive"sv});
    }
    if (item.GetAmount() < 0) {
        return std::unexpected(
            Error{ErrorCode::InvalidData,
                  "Inventory item amount cannot be negative"sv});
    }

    try {
        Transaction txn{*connection_};
        Params params;
        params.append(item.GetProductTypeId());
        params.append(item.GetAmount());
        const auto manufacture_date = item.GetManufactureDate();
        const auto expiration_date = item.GetExpirationDate();
        if (manufacture_date.has_value()) {
            params.append(DateToString(manufacture_date.value()));
        } else {
            params.append(nullptr);
        }
        if (expiration_date.has_value()) {
            params.append(DateToString(expiration_date.value()));
        } else {
            params.append(nullptr);
        }
        params.append(id.value());
        const auto result =
            txn.exec(PreparedStatement::kUpdateInventoryItemById, params);
        txn.commit();

        if (result.affected_rows() == 0) {
            return std::unexpected(
                Error{ErrorCode::NotFound, "Inventory item not found"sv});
        }

        return {};
    } catch (const pqxx::foreign_key_violation& e) {
        return std::unexpected(Error{ErrorCode::ConstraintViolation, e.what()});
    } catch (const pqxx::check_violation& e) {
        return std::unexpected(Error{ErrorCode::InvalidData, e.what()});
    } catch (const pqxx::not_null_violation& e) {
        return std::unexpected(Error{ErrorCode::InvalidData, e.what()});
    } catch (const pqxx::unique_violation& e) {
        return std::unexpected(Error{ErrorCode::ConstraintViolation, e.what()});
    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<void, Error> InventoryItemRepository::Delete(Id id) {
    if (id <= 0) {
        return std::unexpected(Error{ErrorCode::InvalidData,
                                     "Inventory item ID must be positive"});
    }

    try {
        Transaction txn{*connection_};
        const auto result =
            txn.exec(PreparedStatement::kDeleteInventoryItemById, Params{id});
        txn.commit();

        if (result.affected_rows() == 0) {
            return std::unexpected(
                Error{ErrorCode::NotFound, "Inventory item not found"sv});
        }

        return {};

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    } catch (const std::exception& e) {
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
        return std::unexpected(Error{ErrorCode::InvalidData,
                                     "Consumption amount must be positive"});
    }

    try {
        Transaction txn{*connection_};
        const auto result =
            txn.exec(PreparedStatement::kSelectInventoryForConsumption,
                     Params{product_type_id});

        Amount remaining = amount;
        for (const auto& row : result) {
            if (remaining <= 0) {
                break;
            }
            const auto item_id = row["id"].as<Id>();
            const auto available_amount = row["amount_base"].as<Amount>();
            const auto consumed = std::min(available_amount, remaining);
            const auto new_amount = available_amount - consumed;
            if (new_amount == 0) {
                txn.exec(PreparedStatement::kDeleteEmptyInventoryItem,
                         Params{item_id});
            } else {
                txn.exec(PreparedStatement::kUpdateInventoryAmount,
                         Params{new_amount, item_id});
            }
            remaining -= consumed;
        }

        if (remaining > 0) {
            txn.abort();
            return std::unexpected(Error{ErrorCode::NotCookable,
                                         "Insufficient inventory amount"sv});
        }
        txn.commit();
        return {};

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

}  // namespace db::repo::postgresql