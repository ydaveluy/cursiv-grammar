#pragma once

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace xyz {
inline std::optional<std::uint64_t> atom_count(std::string_view text) {
  const bool negative = text.starts_with('-');
  if (text.starts_with('+') || negative)
    text.remove_prefix(1);
  std::string digits;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '_') {
      if (i == 0 || i + 1 == text.size() || text[i - 1] < '0' ||
          text[i - 1] > '9' || text[i + 1] < '0' || text[i + 1] > '9')
        return std::nullopt;
    } else {
      digits += text[i];
    }
  }
  std::uint64_t value = 0;
  const auto [end, error] = std::from_chars(
      digits.data(), digits.data() + digits.size(), value);
  if (error != std::errc{} || end != digits.data() + digits.size() ||
      (negative && value != 0))
    return std::nullopt;
  return value;
}
}  // namespace xyz
