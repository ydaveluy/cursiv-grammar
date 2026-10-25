#pragma once
#include <string_view>
#include "bdf/core/ast.hpp"

namespace bdf {
inline const ast::Directive* find_directive(const auto& parts, std::string_view keyword) {
  const ast::Directive* found = nullptr;
  for (const auto& part : parts)
    if (const auto* p = std::get_if<ast::Directive*>(&part); p && *p && (*p)->keyword == keyword)
      found = *p;
  return found;
}
inline std::string_view argument(const ast::Directive* line, std::size_t index) {
  return line && index < line->arguments.size() ? line->arguments[index] : std::string_view{};
}
inline const ast::Bitmap* bitmap(const ast::Char& glyph) {
  const ast::Bitmap* found = nullptr;
  for (const auto& part : glyph.parts)
    if (const auto* p = std::get_if<ast::Bitmap*>(&part); p && *p) found = *p;
  return found;
}
}  // namespace bdf
