#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace client {

enum class Scheme : uint8_t { Http, Https };

struct ClientConfig final {
    Scheme scheme{Scheme::Http};
    std::string host{"127.0.0.1"};
    uint16_t port{};
    std::string server_name{"localhost"};
    std::filesystem::path ca_file;
    bool verify_peer{true};
    bool keep_alive{true};
    size_t header_limit{8192};
    size_t body_limit{1048576};
    uint32_t timeout_seconds{30};
};

}  // namespace client