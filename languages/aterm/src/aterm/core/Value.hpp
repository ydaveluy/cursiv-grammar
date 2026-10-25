#pragma once
#include "aterm/core/ast.hpp"
#include <string>
namespace aterm {
inline bool quoted(const ast::Function &f) { return f.afun.starts_with('"'); }
inline std::string function_name(const ast::Function &f) {
  const auto raw = f.afun;
  if (!quoted(f))
    return std::string(raw);
  std::string out;
  for (std::size_t i = 1; i + 1 < raw.size(); ++i) {
    char c = raw[i];
    if (c == '\\' && i + 2 < raw.size()) {
      c = raw[++i];
      switch (c) {
      case 'n':
        c = '\n';
        break;
      case 'r':
        c = '\r';
        break;
      case 't':
        c = '\t';
        break;
      default:
        break;
      }
    }
    out += c;
  }
  return out;
}
inline bool real(const ast::Number &n) {
  return n.raw.find_first_of(".eE") != n.raw.npos;
}
} // namespace aterm
