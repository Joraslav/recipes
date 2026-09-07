#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace client {

enum class ErrorCode : uint8_t { Transport, Timeout, Tls, Http, Json };

class Error final {
 public:
    /**
     * @brief Creates an error by taking ownership of its message.
     * @param c Error category.
     * @param msg Human-readable error message.
     * @param st Status of HTTP
     */
    Error(ErrorCode c, std::string msg, std::optional<uint16_t> st)
        : code_(c), message_(std::move(msg)), http_status_(st) {}

    /** @brief Returns the machine-readable error category. */
    [[nodiscard]] ErrorCode GetCode() const noexcept { return code_; }
    /** @brief Returns the human-readable error message. */
    [[nodiscard]] const std::string& GetMessage() const noexcept {
        return message_;
    }
    /**@brief Returns the HTTP-status error. */
    [[nodiscard]] std::optional<uint16_t> GetStatus() const noexcept {
        return http_status_;
    }

 private:
    ErrorCode code_;
    std::string message_;
    std::optional<uint16_t> http_status_;
};

}  // namespace client