#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace db::repo {

/**
 * @brief Error codes representing various failure conditions in the repository.
 */
enum class ErrorCode : uint8_t {
    NotFound,
    AlreadyExists,
    InvalidData,
    DatabaseError,
    ConstraintViolation,
    NotCookable
};

/**
 * @brief Represents an error in the repository, encapsulating an error code and
 * a descriptive message.
 * @param code The error code representing the type of error.
 * @param message A descriptive message providing details about the error.
 */
class Error final {
 public:
    explicit Error(ErrorCode code, std::string_view message)
        : code_(code), message_(message) {}
    Error(const Error&) = delete;
    Error& operator=(const Error&) = delete;
    Error(Error&&) = default;
    Error& operator=(Error&&) = default;
    ~Error() = default;

    [[nodiscard]] ErrorCode GetCode() const noexcept { return code_; }
    [[nodiscard]] const std::string& GetMessage() const noexcept {
        return message_;
    }

 private:
    ErrorCode code_;
    std::string message_;
};

}  // namespace db::repo