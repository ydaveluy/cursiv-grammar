#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gff3 {

inline int hex_value(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

inline std::optional<std::string> attribute_value(std::string_view raw) {
  std::string value;
  for (std::size_t i = 0; i < raw.size(); ++i) {
    if (raw[i] != '%')
      value += raw[i];
    else {
      if (i + 2 >= raw.size() || hex_value(raw[i + 1]) < 0 ||
          hex_value(raw[i + 2]) < 0)
        return std::nullopt;
      value +=
          static_cast<char>(hex_value(raw[i + 1]) * 16 + hex_value(raw[i + 2]));
      i += 2;
    }
  }
  return value;
}

inline std::optional<std::string_view> decimal(std::string_view raw) {
  if (raw.starts_with('+'))
    raw.remove_prefix(1);
  if (raw.empty() || raw.find_first_not_of("0123456789") != raw.npos)
    return std::nullopt;
  const auto first = raw.find_first_not_of('0');
  return first == raw.npos ? raw.substr(raw.size() - 1) : raw.substr(first);
}

// Work on stored fields, before decoding: escaped spaces are part of a Target
// identifier, while literal spaces separate its coordinates.
inline std::vector<std::string_view> words(std::string_view raw) {
  std::vector<std::string_view> result;
  while (!raw.empty()) {
    const auto start = raw.find_first_not_of(' ');
    if (start == raw.npos) break;
    raw.remove_prefix(start);
    const auto end = raw.find(' ');
    result.push_back(raw.substr(0, end));
    if (end == raw.npos) break;
    raw.remove_prefix(end);
  }
  return result;
}

inline bool encoded_field(std::string_view raw, bool seqid = false) {
  constexpr std::string_view id_chars =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.:^*$@!+_?-|";
  for (std::size_t i = 0; i < raw.size(); ++i) {
    const auto c = static_cast<unsigned char>(raw[i]);
    if (c == '%') {
      if (i + 2 >= raw.size() || hex_value(raw[i + 1]) < 0 ||
          hex_value(raw[i + 2]) < 0) return false;
      i += 2;
    } else if (c < 0x20 || c == 0x7f ||
               (seqid && id_chars.find(static_cast<char>(c)) == id_chars.npos)) {
      return false;
    }
  }
  return true;
}

inline bool ordered_coordinates(std::string_view begin, std::string_view end) {
  const auto a = decimal(begin), b = decimal(end);
  return a && b && *a != "0" && *b != "0" &&
         (a->size() < b->size() || (a->size() == b->size() && *a <= *b));
}

} // namespace gff3
