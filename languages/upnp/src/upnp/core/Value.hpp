#pragma once
#include "upnp/core/ast.hpp"
#include <string>
namespace upnp {
inline std::string comparison_value(const ast::Comparison &c) {
  const auto raw = c.raw;
  if (raw.size() < 2)
    return {};
  const auto body = raw.substr(1, raw.size() - 2);
  if (raw.front() == '\'')
    return std::string(body);
  std::string value;
  value.reserve(body.size());
  for (std::size_t i = 0; i < body.size(); ++i) {
    char ch = body[i];
    if (ch == '\\' && i + 1 < body.size()) {
      ch = body[++i];
      switch (ch) {
      case 'n':
        ch = '\n';
        break;
      case 't':
        ch = '\t';
        break;
      case 'r':
        ch = '\r';
        break;
      case 'b':
        ch = '\b';
        break;
      case 'f':
        ch = '\f';
        break;
      default:
        if (ch >= '0' && ch <= '7') {
          unsigned octal = ch - '0';
          for (unsigned n = 1; n < 3 && i + 1 < body.size() &&
                               body[i + 1] >= '0' && body[i + 1] <= '7';
               ++n)
            octal = octal * 8 + body[++i] - '0';
          ch = static_cast<char>(octal & 255u);
        }
      }
    }
    value += ch;
  }
  return value;
}
} // namespace upnp
