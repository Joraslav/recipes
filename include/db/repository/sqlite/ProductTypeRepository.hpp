#pragma once

#include "SQLiteCpp/Database.h"

#include "Error.hpp"
#include "IProductTypeRepository.hpp"
#include "types/Defines.hpp"
#include "types/kitchen/Defines.hpp"
#include "types/kitchen/ProductType.hpp"

#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace db::repo::sqlite {

class ProductTypeRepository final : public repo::IProductTypeRepository {
 public:
    using Database = SQLite::Database;

    explicit ProductTypeRepository(SQLite::Database* database)
        : database_(database) {
        if (database_ == nullptr) {
            throw std::invalid_argument("Database pointer cannot be null");
        }
    }

    [[nodiscard]]
    types::Result<types::Id, repo::Error> Insert(
        const types::ProductType& product) override;

    [[nodiscard]]
    types::Result<std::optional<types::ProductType>, repo::Error> GetById(
        types::Id id) override;

    [[nodiscard]]
    types::Result<std::optional<types::ProductType>, repo::Error> GetByName(
        std::string_view name) override;

    [[nodiscard]]
    types::Result<std::vector<types::ProductType>, repo::Error> GetAll()
        override;

    [[nodiscard]]
    types::Result<void, repo::Error> Update(
        const types::ProductType& product) override;

    [[nodiscard]]
    types::Result<void, repo::Error> Delete(types::Id id) override;

 private:
    Database* database_;
};

}  // namespace db::repo::sqlite