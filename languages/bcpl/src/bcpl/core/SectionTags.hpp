#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/syntax-tree/SourceTranslation.hpp"

namespace bcpl {
namespace section_detail {
inline bool space(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}
inline std::size_t tag_end(std::string_view source, std::size_t at) {
  while (at < source.size()) {
    const char c = source[at];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '_' ||
        (c == '.' && (at + 1 == source.size() || source[at + 1] != '.'))) ++at;
    else break;
  }
  return at;
}
inline std::size_t line_comment_end(std::string_view source, std::size_t at) {
  while (at < source.size() && source[at] != '\r' && source[at] != '\n') ++at;
  return at;
}
inline std::size_t quoted_end(std::string_view source, std::size_t at) {
  const char quote = source[at++];
  while (at < source.size()) {
    if (source[at] == quote) return at + 1;
    if (source[at] != '*') { ++at; continue; }
    // Native * whitespace-or-//comment * continuation: a quote or bracket
    // inside that comment is inert, rather than the end of this string.
    std::size_t next = at + 1;
    bool trivia = false;
    while (next < source.size()) {
      if (space(source[next])) { ++next; trivia = true; }
      else if (source.substr(next).starts_with("//")) {
        next = line_comment_end(source, next + 2);
        trivia = true;
      } else break;
    }
    if (trivia && next < source.size() && source[next] == '*') at = next + 1;
    else at += at + 1 < source.size() ? 2 : 1;  // ordinary * escape
  }
  return at;
}
}  // namespace section_detail

// Richards §2.1.4: a tagged close also closes sections nested inside the
// matching opener. Keep the written tags; insert only the implicit closers.
inline void translate_section_tags(std::string_view source,
                                   cursiv::SourceTranslation& out) {
  std::vector<std::string_view> stack;
  for (std::size_t at = 0; at < source.size();) {
    if (source.substr(at).starts_with("//")) {
      at = section_detail::line_comment_end(source, at + 2);
      continue;
    }
    if (source.substr(at).starts_with("/*")) {
      at += 2;
      std::size_t depth = 1;
      while (at < source.size() && depth) {
        if (source.substr(at).starts_with("/*")) { ++depth; at += 2; }
        else if (source.substr(at).starts_with("*/")) { --depth; at += 2; }
        else ++at;
      }
      continue;
    }
    if (source[at] == '"' || source[at] == '\'') {
      at = section_detail::quoted_end(source, at);
      continue;
    }
    const bool dollarOpen = source.substr(at).starts_with("$(");
    const bool dollarClose = source.substr(at).starts_with("$)");
    if (!dollarOpen && !dollarClose && source[at] != '{' && source[at] != '}') {
      ++at;
      continue;
    }
    const auto end = dollarOpen || dollarClose
        ? section_detail::tag_end(source, at + 2) : at + 1;
    const auto tag = dollarOpen || dollarClose
        ? source.substr(at + 2, end - at - 2) : std::string_view{};
    if (dollarOpen || source[at] == '{') stack.push_back(tag);
    else if (tag.empty()) {
      if (stack.empty()) out.error(at, end, "Section closer has no opener.");
      else {
        if (!stack.back().empty())
          out.error(at, end, "A tagged BCPL section needs a tagged closer.");
        stack.pop_back();
      }
    } else {
      std::size_t match = stack.size();
      while (match && stack[match - 1] != tag) --match;
      if (!match) {
        // Conservative local diagnostic. Historical front ends may recover
        // differently here; no native rejection equivalence is claimed.
        out.error(at, end, "Section tag has no matching open section.");
      } else {
        std::string closers;
        for (std::size_t i = match; i < stack.size(); ++i) closers += "} ";
        if (!closers.empty()) out.replace(at, at, closers);
        stack.resize(match - 1);
      }
    }
    at = end;
  }
}
}  // namespace bcpl
