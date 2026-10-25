#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace rfc1960 {

inline int hex_value(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

inline std::optional<std::string> filter_value(std::string_view raw) {
  std::string value;
  for (std::size_t i = 0; i < raw.size(); ++i) {
    if (raw[i] == '\\') {
      if (++i == raw.size())
        return std::nullopt;
      const int high = hex_value(raw[i]);
      if (high >= 0) {
        if (i + 1 == raw.size() || hex_value(raw[i + 1]) < 0)
          return std::nullopt;
        value += static_cast<char>(high * 16 + hex_value(raw[++i]));
      } else if (raw[i] == '(' || raw[i] == ')' || raw[i] == '*' ||
                 raw[i] == '\\') {
        value += raw[i];
      } else
        return std::nullopt;
    } else if (raw[i] == '(' || raw[i] == ')' || raw[i] == '*' ||
               raw[i] == '\0') {
      return std::nullopt;
    } else
      value += raw[i];
  }
  return value;
}

inline bool oid(std::string_view text) {
  const auto alpha = [](char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
  };
  const auto digit = [](char c) { return c >= '0' && c <= '9'; };
  if (text.empty())
    return false;
  if (alpha(text.front())) {
    for (char c : text)
      if (!alpha(c) && !digit(c) && c != '-')
        return false;
    return true;
  }
  if (!digit(text.front()) || text.back() == '.')
    return false;
  bool dot = false;
  for (char c : text) {
    if (c == '.') {
      if (dot)
        return false;
      dot = true;
    } else {
      if (!digit(c))
        return false;
      dot = false;
    }
  }
  return true;
}

inline bool attribute_name(std::string_view text) {
  auto end = text.find(';');
  if (!oid(text.substr(0, end)))
    return false;
  while (end != text.npos) {
    text.remove_prefix(end + 1);
    end = text.find(';');
    const auto option = text.substr(0, end);
    if (option.empty())
      return false;
    for (char c : option)
      if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-'))
        return false;
  }
  return true;
}

inline bool dn_qualifier(std::string_view text) {
  return text.size() == 2 && (text[0] == 'd' || text[0] == 'D') &&
         (text[1] == 'n' || text[1] == 'N');
}

} // namespace rfc1960
