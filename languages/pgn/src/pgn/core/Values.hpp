#pragma once

#include <string_view>

#include "pgn/core/ast.hpp"

namespace pgn {
inline std::string_view comment_value(std::string_view text) {
  if (text.empty()) return {};
  text.remove_prefix(1);
  const bool closed = text.ends_with('}');
  if (closed) text.remove_suffix(1);
  if (text.starts_with(' ')) text.remove_prefix(1);
  if (closed && text.ends_with(' ')) text.remove_suffix(1);
  return text;
}
inline std::string_view result_value(ast::Result result) {
  switch (result) {
    case ast::Result::white_wins:
      return "1-0";
    case ast::Result::black_wins:
      return "0-1";
    case ast::Result::drawn:
      return "1/2-1/2";
    default:
      return "*";
  }
}
}  // namespace pgn
