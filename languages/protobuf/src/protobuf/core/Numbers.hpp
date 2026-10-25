#pragma once
#include "protobuf/core/ast.hpp"
#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>
namespace protobuf {
using Number = std::variant<std::int64_t, std::uint64_t, double>;
std::optional<Number> number_value(std::string_view text,
                                   bool floating = false);
std::optional<std::int64_t> integer_value(std::string_view text);
} // namespace protobuf
