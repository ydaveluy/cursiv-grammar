#pragma once
#include <cstdint>
#include <optional>
#include <string_view>
namespace yara {
std::optional<std::int64_t> integerValue(std::string_view text);
}
