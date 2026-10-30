#pragma once

#include <string>
#include <string_view>

#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/utils/Utf8.hpp"
#include "inf/core/ast.hpp"

namespace inf {

inline std::size_t space_size(std::string_view text, std::size_t at) {
  if (at >= text.size()) return 0;
  const std::size_t width = cursiv::utf8_width(text[at]);
  static constexpr std::string_view spaces =
      "\t\v\f \u0085\u00a0\u1680\u2000\u2001\u2002\u2003\u2004\u2005"
      "\u2006\u2007\u2008\u2009\u200a\u2028\u2029\u202f\u205f\u3000";
  return spaces.find(text.substr(at, width)) == std::string_view::npos ? 0
                                                                       : width;
}

inline std::string decode_value(const ast::Item& item, const cursiv::Cst& cst) {
  const auto text = item.raw;
  const auto source = cst.text();
  const auto tail = cst[item.cstNodeId].end;
  std::string result, pending;
  bool quoted = false;
  bool leading = true;
  for (std::size_t i = 0; i < text.size();) {
    const char c = text[i];
    if (quoted) {
      if (c == '"') {
        if (i + 1 < text.size() && text[i + 1] == '"') {
          result += '"';
          i += 2;
        } else {
          quoted = false;
          ++i;
        }
      } else
        result += text[i++];
      continue;
    }
    if (c == '"') {
      pending.clear();
      quoted = true;
      leading = false;
      ++i;
      continue;
    }
    if (c == '\\') {
      std::size_t end = i + 1;
      while (end < text.size()) {
        if (text[end] == '\\')
          ++end;
        else if (const auto width = space_size(text, end))
          end += width;
        else
          break;
      }
      if (end < text.size() && text[end] == ';')
        while (end < text.size() && text[end] != '\n' && text[end] != '\r') ++end;
      const bool atEof = tail == source.size() ||
                         (tail < source.size() &&
                          (source[tail] == '\0' || source[tail] == '\x1a'));
      if ((end < text.size() && (text[end] == '\n' || text[end] == '\r')) ||
          (end == text.size() && atEof)) {
        pending.clear();
        leading = true;
        i = end < text.size() ? end + 1 : end;
        if (end < text.size() && text[end] == '\r' &&
            i < text.size() && text[i] == '\n') ++i;
        while (const auto width = space_size(text, i)) i += width;
        continue;
      }
    }
    if (const auto width = space_size(text, i)) {
      if (!leading) pending.append(text.substr(i, width));
      i += width;
    } else {
      result += pending;
      pending.clear();
      leading = false;
      const auto scalarWidth = cursiv::utf8_width(text[i]);
      result.append(text.substr(i, scalarWidth));
      i += scalarWidth;
    }
  }
  return result;
}

}  // namespace inf
