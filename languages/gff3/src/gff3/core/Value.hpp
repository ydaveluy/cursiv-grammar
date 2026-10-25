#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace gff3 {

inline int hex_value(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

inline std::optional<std::string> attribute_value(std::string_view raw) {
  std::string value;
  for (std::size_t i = 0; i < raw.size(); ++i) {
    if (raw[i] != '%')
      value += raw[i];
    else {
      if (i + 2 >= raw.size() || hex_value(raw[i + 1]) < 0 ||
          hex_value(raw[i + 2]) < 0)
        return std::nullopt;
      value +=
          static_cast<char>(hex_value(raw[i + 1]) * 16 + hex_value(raw[i + 2]));
      i += 2;
    }
  }
  return value;
}

inline std::optional<std::string_view> decimal(std::string_view raw) {
  if (raw.starts_with('+'))
    raw.remove_prefix(1);
  if (raw.empty() || raw.find_first_not_of("0123456789") != raw.npos)
    return std::nullopt;
  const auto first = raw.find_first_not_of('0');
  return first == raw.npos ? raw.substr(raw.size() - 1) : raw.substr(first);
}

} // namespace gff3
