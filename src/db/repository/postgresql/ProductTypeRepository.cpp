#include "ProductTypeRepository.hpp"

#include "Error.hpp"
#include "SQL/PostgreSQLStatements.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/ProductType.hpp"

#include <cstdint>
#include <exception>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "pqxx/result"
#include "pqxx/row"
#include "pqxx/transaction"

using db::repo::Error;
using db::repo::ErrorCode;

using types::Dimension;
using types::Id;
using types::ProductType;
using types::Result;

using Params = pqxx::params;
using Transaction = pqxx::transaction<>;

using PreparedStatement = db::stmt::PostgreSQLStatements;

using namespace std::string_view_literals;

namespace {

[[nodiscard]] ProductType MakeProductTypeFromRow(const pqxx::row_ref& row) {
    const auto id = row["id"].as<Id>();
    const auto name = row["name"].as<std::string>();
    const auto dimension = static_cast<Dimension>(row["dimension"].as<int>());

    return ProductType{name, dimension, id};
}

}  // namespace

namespace db::repo::postgresql {

Result<Id, Error> ProductTypeRepository::Insert(const ProductType& product) {
    try {
        Transaction txn{*connection_};
        const auto result =
            txn.exec(PreparedStatement::kInsertProductType,
                     Params{product.GetName(),
                            static_cast<uint8_t>(product.GetDimension())});
        txn.commit();

        return result[0]["id"].as<Id>();

    } catch (const pqxx::unique_violation& e) {
        return std::unexpected(Error{ErrorCode::AlreadyExists, e.what()});
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

Result<std::optional<ProductType>, Error> ProductTypeRepository::GetById(
    Id id) {
    try {
        Transaction txn{*connection_};
        const auto result =
            txn.exec(PreparedStatement::kSelectProductTypeById, Params{id});
        txn.commit();

        if (result.empty()) {
            return std::nullopt;
        }

        return MakeProductTypeFromRow(result[0]);
    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<std::optional<ProductType>, Error> ProductTypeRepository::GetByName(
    std::string_view name) {
    try {
        Transaction txn{*connection_};
        const auto result =
            txn.exec(PreparedStatement::kSelectProductTypeByName, Params{name});
        txn.commit();

        if (result.empty()) {
            return std::nullopt;
        }

        return MakeProductTypeFromRow(result[0]);

    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<std::vector<ProductType>, Error> ProductTypeRepository::GetAll() {
    try {
        Transaction txn{*connection_};
        const auto result = txn.exec(PreparedStatement::kSelectAllProductTypes);
        txn.commit();

        std::vector<ProductType> product_types;
        product_types.reserve(result.size());
        for (const auto& row : result) {
            product_types.emplace_back(MakeProductTypeFromRow(row));
        }

        return product_types;
    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<void, Error> ProductTypeRepository::Update(const ProductType& product) {
    if (!product.GetId().has_value()) {
        return std::unexpected(
            Error{ErrorCode::InvalidData,
                  "Product type ID is required for update"sv});
    }

    try {
        Transaction txn{*connection_};
        const auto result = txn.exec(
            PreparedStatement::kUpdateProductTypeById,
            Params{product.GetName(), static_cast<int>(product.GetDimension()),
                   product.GetId().value()});
        txn.commit();

        if (result.affected_rows() == 0) {
            return std::unexpected(
                Error{ErrorCode::NotFound,
                      "Product type with specified ID was not found"sv});
        }

        return {};

    } catch (const pqxx::unique_violation& e) {
        return std::unexpected(Error{ErrorCode::AlreadyExists, e.what()});
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

Result<void, Error> ProductTypeRepository::Delete(Id id) {
    try {
        Transaction txn{*connection_};
        const auto result =
            txn.exec(PreparedStatement::kDeleteProductTypeById, Params{id});
        txn.commit();

        if (result.affected_rows() == 0) {
            return std::unexpected(
                Error{ErrorCode::NotFound,
                      "Product type with specified ID was not found"sv});
        }

        return {};
    } catch (const pqxx::foreign_key_violation& e) {
        return std::unexpected(Error{ErrorCode::ConstraintViolation, e.what()});
    } catch (const pqxx::sql_error& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

}  // namespace db::repo::postgresql