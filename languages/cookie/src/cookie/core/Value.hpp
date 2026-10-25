#pragma once

#include <optional>
#include <string>
#include "cookie/core/ast.hpp"

namespace cookie {
inline std::optional<std::string> decoded_value(const ast::Pair& pair) {
  if (!pair.value)
    return std::nullopt;
  std::string result;
  const auto text = *pair.value;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (pair.quoted && text[i] == '\\' && i + 1 < text.size()) {
      if (i + 3 < text.size() && text[i + 1] >= '0' && text[i + 1] <= '3' &&
          text[i + 2] >= '0' && text[i + 2] <= '7' &&
          text[i + 3] >= '0' && text[i + 3] <= '7') {
        result += static_cast<char>((text[i + 1] - '0') * 64 +
                                    (text[i + 2] - '0') * 8 + text[i + 3] - '0');
        i += 3;
        continue;
      }
      ++i;
    }
    result += text[i];
  }
  return result;
}
}  // namespace cookie
