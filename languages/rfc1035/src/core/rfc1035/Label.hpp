#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace rfc1035 {

inline std::optional<std::string> label_value(std::string_view raw) {
  std::string value;
  for (std::size_t i = 0; i < raw.size(); ++i) {
    if (raw[i] != '\\') {
      value += raw[i];
      continue;
    }
    if (++i == raw.size()) return std::nullopt;
    if (raw[i] < '0' || raw[i] > '9') {
      value += raw[i];
      continue;
    }
    unsigned byte = 0;
    for (unsigned j = 0; j < 3; ++j, ++i) {
      if (i == raw.size() || raw[i] < '0' || raw[i] > '9')
        return std::nullopt;
      byte = byte * 10 + static_cast<unsigned>(raw[i] - '0');
    }
    if (byte > 255) return std::nullopt;
    value += static_cast<char>(byte);
    --i;
  }
  return value;
}

}  // namespace rfc1035
