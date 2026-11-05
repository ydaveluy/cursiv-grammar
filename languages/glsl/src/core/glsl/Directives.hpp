#pragma once
#include <cctype>
#include <string>
#include <string_view>

namespace glsl {
struct DirectiveText {
  std::string name;
  std::string arguments;
};
inline DirectiveText read_directive(std::string_view source) {
  std::string text;
  bool quoted = false;
  for (std::size_t i = 0; i < source.size(); ++i) {
    const char c = source[i];
    if (c == '\\' && i + 1 < source.size()) {
      if (source[i + 1] == '\n') {
        ++i;
        continue;
      }
      if (source[i + 1] == '\r') {
        ++i;
        if (i + 1 < source.size() && source[i + 1] == '\n')
          ++i;
        continue;
      }
      if (quoted) {
        text += c;
        text += source[++i];
        continue;
      }
    }
    if (c == '"')
      quoted = !quoted;
    if (!quoted && c == '/' && i + 1 < source.size()) {
      if (source[i + 1] == '/')
        break;
      if (source[i + 1] == '*') {
        const auto end = source.find("*/", i + 2);
        text += ' ';
        i = end == source.npos ? source.size() - 1 : end + 1;
        continue;
      }
    }
    text += c;
  }
  const auto space = [](char c) {
    return std::isspace(static_cast<unsigned char>(c));
  };
  std::size_t i = text.find('#');
  i = i == text.npos ? 0 : i + 1;
  while (i < text.size() && space(text[i]))
    ++i;
  const auto start = i;
  while (i < text.size() &&
         (std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_'))
    ++i;
  DirectiveText result{text.substr(start, i - start), {}};
  while (i < text.size() && space(text[i]))
    ++i;
  auto end = text.size();
  while (end > i && space(text[end - 1]))
    --end;
  result.arguments = text.substr(i, end - i);
  return result;
}
} // namespace glsl
