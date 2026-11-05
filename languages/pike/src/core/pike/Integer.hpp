#pragma once
#include <optional>
#include <string>
#include <string_view>
namespace pike {
std::optional<std::string> integer_value(std::string_view text);
}
