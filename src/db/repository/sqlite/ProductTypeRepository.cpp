#include "ProductTypeRepository.hpp"

#include "Error.hpp"
#include "SQL/SQLiteStatements.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/ProductType.hpp"

#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using db::repo::Error;
using db::repo::ErrorCode;

using types::Dimension;
using types::Id;
using types::ProductType;
using types::Result;

using Statement = SQLite::Statement;
using PreparedStatement = db::stmt::SQLiteStatements;

using namespace std::string_view_literals;

namespace db::repo::sqlite {

Result<Id, Error> ProductTypeRepository::Insert(const ProductType& product) {
    try {
        Statement statement{*database_,
                            std::string(PreparedStatement::kInsertProductType)};

        statement.bind(1, product.GetName());
        statement.bind(2, static_cast<uint8_t>(product.GetDimension()));

        if (!statement.executeStep()) {
            return std::unexpected(Error{ErrorCode::DatabaseError,
                                         "Failed to insert product type"sv});
        }

        return statement.getColumn(0).getInt64();
    } catch (const SQLite::Exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<std::optional<ProductType>, Error> ProductTypeRepository::GetById(
    Id id) {
    try {
        Statement statement{
            *database_, std::string(PreparedStatement::kSelectProductTypeById)};

        statement.bind(1, id);

        if (!statement.executeStep()) {
            return std::optional<ProductType>{std::nullopt};
        }

        const Id product_id = statement.getColumn(0).getInt64();
        const std::string name = statement.getColumn(1).getString();
        const Dimension dimension =
            static_cast<Dimension>(statement.getColumn(2).getUInt());

        return ProductType{name, dimension, product_id};
    } catch (const SQLite::Exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<std::optional<ProductType>, Error> ProductTypeRepository::GetByName(
    std::string_view name) {
    try {
        Statement statement{
            *database_,
            std::string(PreparedStatement::kSelectProductTypeByName)};

        statement.bind(1, std::string(name));

        if (!statement.executeStep()) {
            return std::optional<ProductType>{std::nullopt};
        }

        const Id id = statement.getColumn(0).getInt64();
        const std::string product_name = statement.getColumn(1).getString();
        const Dimension dimension =
            static_cast<Dimension>(statement.getColumn(2).getInt());

        return ProductType{product_name, dimension, id};
    } catch (const SQLite::Exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
}

Result<std::vector<ProductType>, Error> ProductTypeRepository::GetAll() {
    try {
        Statement statement{
            *database_, std::string(PreparedStatement::kSelectAllProductTypes)};

        std::vector<ProductType> product_types;
        while (statement.executeStep()) {
            const Id id = statement.getColumn(0).getInt64();
            const std::string name = statement.getColumn(1).getString();
            const Dimension dimension =
                static_cast<Dimension>(statement.getColumn(2).getInt());

            product_types.emplace_back(name, dimension, id);
        }

        return product_types;
    } catch (const SQLite::Exception& e) {
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
        Statement statement{
            *database_, std::string(PreparedStatement::kUpdateProductTypeById)};

        statement.bind(1, product.GetName());
        statement.bind(2, static_cast<uint8_t>(product.GetDimension()));
        statement.bind(3, product.GetId().value());

        std::ignore = statement.exec();

        if (database_->getChanges() == 0) {
            return std::unexpected(
                Error{ErrorCode::NotFound, "Product type not found"sv});
        }
    } catch (const SQLite::Exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
    return {};
}

Result<void, Error> ProductTypeRepository::Delete(Id id) {
    try {
        Statement statement{
            *database_, std::string(PreparedStatement::kDeleteProductTypeById)};

        statement.bind(1, id);

        std::ignore = statement.exec();

        if (database_->getChanges() == 0) {
            return std::unexpected(
                Error{ErrorCode::NotFound, "Product type not found"sv});
        }

    } catch (const SQLite::Exception& e) {
        return std::unexpected(Error{ErrorCode::DatabaseError, e.what()});
    }
    return {};
}

}  // namespace db::repo::sqlite