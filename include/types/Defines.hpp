#pragma once

#include <expected>

namespace types {

template <typename T, typename E>
using Result = std::expected<T, E>;

}  // namespace types