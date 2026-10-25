#pragma once
#include "star/core/ast.hpp"
#include <string>
#include <string_view>
#include <vector>
namespace star {
enum class StringSyntax { cif1, star2, nmr };
inline std::vector<std::string_view> loop_columns(const ast::LoopHeader &h) {
  std::vector<std::string_view> names;
  for (const auto &item : h.items)
    if (auto *name = std::get_if<std::string_view>(&item))
      names.push_back(*name);
  return names;
}
inline bool nested_header(const ast::LoopHeader &h) {
  for (const auto &item : h.items)
    if (std::holds_alternative<ast::LoopHeader *>(item))
      return true;
  return false;
}
inline std::string_view string_body(const ast::StringLit &value,
                                    StringSyntax syntax = StringSyntax::star2) {
  const auto raw = value.raw;
  if (raw.starts_with('\n') || raw.starts_with('\r')) {
    const auto begin = raw.find(';') + 1;
    auto end = raw.size() - 1;
    if (end && raw[end - 1] == '\n') {
      --end;
      if (end && raw[end - 1] == '\r')
        --end;
    } else if (end && raw[end - 1] == '\r')
      --end;
    return begin <= end ? raw.substr(begin, end - begin) : std::string_view{};
  }
  const std::size_t width =
      syntax == StringSyntax::star2 &&
              (raw.starts_with("'''") || raw.starts_with("\"\"\""))
          ? 3
          : 1;
  return raw.size() >= 2 * width ? raw.substr(width, raw.size() - 2 * width)
                                 : std::string_view{};
}
inline std::string string_value(const ast::StringLit &value,
                                StringSyntax syntax = StringSyntax::star2) {
  std::string text(string_body(value, syntax));
  if (syntax == StringSyntax::nmr &&
      (value.raw.starts_with('\n') || value.raw.starts_with('\r'))) {
    std::string normalized;
    for (std::size_t i = 0; i < text.size(); ++i) {
      if (text[i] == '\r') {
        normalized += '\n';
        if (i + 1 < text.size() && text[i + 1] == '\n')
          ++i;
      } else
        normalized += text[i];
    }
    const bool nonempty = !normalized.empty();
    if (normalized.starts_with('\n'))
      normalized.erase(0, 1);
    if (nonempty)
      normalized += '\n';
    if (normalized.starts_with("\n   ") &&
        normalized.find("\n   ;") != normalized.npos) {
      bool indented = true;
      std::size_t pos = 1;
      while (pos < normalized.size()) {
        auto end = normalized.find('\n', pos);
        if (end == normalized.npos)
          end = normalized.size();
        if (end > pos && normalized.compare(pos, 3, "   ") != 0) {
          indented = false;
          break;
        }
        pos = end + 1;
      }
      if (indented) {
        while (normalized.ends_with('\n'))
          normalized.pop_back();
        std::size_t at = 0;
        while ((at = normalized.find("\n   ", at)) != normalized.npos) {
          normalized.erase(at + 1, 3);
          ++at;
        }
      }
    }
    return normalized;
  }
  if (!value.raw.starts_with('\n') && !value.raw.starts_with('\r'))
    return text;
  auto padding = [](char c) {
    return c == ' ' || c == '\t' || c == '\v' || c == '\f';
  };
  const auto firstEnd = text.find('\n'), slash = text.find('\\');
  std::size_t prefixChars = 0;
  if (slash != text.npos)
    for (std::size_t i = 0; i < slash; ++i)
      if ((static_cast<unsigned char>(text[i]) & 0xc0) != 0x80)
        ++prefixChars;
  if (firstEnd != text.npos && prefixChars >= 2 && slash < firstEnd &&
      text.front() != ';' && text.front() != '\n') {
    std::size_t pos = slash;
    while (pos < firstEnd && text[pos] == '\\')
      ++pos;
    const auto slashes = pos - slash;
    while (pos < firstEnd && padding(text[pos]))
      ++pos;
    if ((slashes == 1 || slashes == 2) && pos == firstEnd) {
      const auto prefix = text.substr(0, slash);
      std::string result;
      if (slashes == 2)
        result = "\\\n";
      pos = firstEnd + 1;
      while (pos < text.size()) {
        if (text.compare(pos, prefix.size(), prefix) == 0)
          pos += prefix.size();
        auto end = text.find('\n', pos);
        if (end == text.npos) {
          result.append(text, pos, text.size() - pos);
          break;
        }
        result.append(text, pos, end - pos + 1);
        pos = end + 1;
      }
      text = std::move(result);
    }
  }
  auto first = text.find('\n');
  if (first == text.npos || text.empty() || text.front() != '\\')
    return text;
  for (std::size_t i = 1; i < first; ++i)
    if (!padding(text[i]))
      return text;
  std::string result;
  std::size_t pos = 0;
  while (pos < text.size()) {
    auto end = text.find('\n', pos);
    const bool newline = end != text.npos;
    if (!newline)
      end = text.size();
    auto tail = end;
    while (tail > pos && padding(text[tail - 1]))
      --tail;
    if (tail > pos && text[tail - 1] == '\\')
      result.append(text, pos, tail - pos - 1);
    else {
      result.append(text, pos, end - pos);
      if (newline)
        result += '\n';
    }
    pos = end + (newline ? 1 : 0);
  }
  return result;
}
} // namespace star
