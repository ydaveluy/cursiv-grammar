#pragma once

#include "javadoc/core/ast.hpp"
#include <string>

namespace javadoc {

inline std::string inline_content(const ast::InlineTag &tag) {
  const cursiv::AstNode *owner = tag.parent;
  while (owner && !cursiv::ast_cast<ast::Documentation>(owner))
    owner = owner->parent;
  const auto *doc = cursiv::ast_cast<ast::Documentation>(owner);
  std::string text;
  const auto raw = tag.content;
  for (std::size_t i = 0; i < raw.size(); ++i) {
    const char c = raw[i];
    if (c != '\r' && c != '\n') {
      text += c;
      continue;
    }
    text += '\n';
    if (c == '\r' && i + 1 < raw.size() && raw[i + 1] == '\n')
      ++i;
    if (doc && doc->wrapped) {
      auto next = i + 1;
      while (next < raw.size() && (raw[next] == ' ' || raw[next] == '\t'))
        ++next;
      if (next < raw.size() && raw[next] == '*') {
        while (next < raw.size() && raw[next] == '*')
          ++next;
        i = next - 1;
      }
    }
  }
  if (tag.name == "code" || tag.name == "literal") {
    if (!text.empty() && text.front() == ' ')
      text.erase(0, 1);
  } else {
    const auto begin = text.find_first_not_of(" \t\r\n\f");
    text.erase(0, begin == text.npos ? text.size() : begin);
  }
  return text;
}

} // namespace javadoc
