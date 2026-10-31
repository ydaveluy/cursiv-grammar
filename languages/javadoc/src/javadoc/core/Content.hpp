#pragma once

#include "javadoc/core/ast.hpp"
#include <string>
#include <vector>

namespace javadoc {

namespace detail {
inline void push_parts(std::vector<ast::Item>& work, const auto& parts) {
  for (auto i = parts.size(); i > 0; --i) work.push_back(parts[i - 1]);
}

// Source projection from the typed tree. One buffer and a heap worklist keep
// native stack usage independent of nesting; no CST/source span is scanned.
inline void append_worklist(std::string& text, std::vector<ast::Item>& work) {
  while (!work.empty()) {
    const auto item = work.back();
    work.pop_back();
    if (const auto* raw = std::get_if<std::string_view>(&item)) text += *raw;
    else if (const auto* value = std::get_if<ast::InlineTag*>(&item)) {
      if (!*value) continue;
      text += (*value)->open;
      text += (*value)->name;
      work.emplace_back((*value)->close);
      if ((*value)->structured) push_parts(work, (*value)->parts);
      else text += (*value)->content;
    } else if (const auto* value = std::get_if<ast::InlineBrace*>(&item)) {
      if (!*value) continue;
      text += (*value)->open;
      work.emplace_back((*value)->close);
      push_parts(work, (*value)->parts);
    }
  }
}
}  // namespace detail

inline void append_item_source(std::string& text, const ast::Item& item) {
  std::vector<ast::Item> work{item};
  detail::append_worklist(text, work);
}

inline void append_inline_content(std::string& text, const ast::InlineTag& tag) {
  if (tag.structured) {
    std::vector<ast::Item> work;
    detail::push_parts(work, tag.parts);
    detail::append_worklist(text, work);
  }
  else text += tag.content;
}

inline std::string raw_inline_content(const ast::InlineTag& tag) {
  std::string text;
  append_inline_content(text, tag);
  return text;
}

inline std::string inline_content(const ast::InlineTag &tag) {
  const cursiv::AstNode *owner = tag.parent;
  while (owner && !cursiv::ast_cast<ast::Documentation>(owner))
    owner = owner->parent;
  const auto *doc = cursiv::ast_cast<ast::Documentation>(owner);
  std::string text;
  const auto raw = raw_inline_content(tag);
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
