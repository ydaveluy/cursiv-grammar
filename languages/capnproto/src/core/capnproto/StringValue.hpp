#pragma once
#include <string>

#include "cursiv/grammar/GrammarSpec.hpp"
namespace capnproto {
inline constexpr auto decode_text = [](std::string_view t) -> std::string {
  if (t.starts_with('`')) {
    auto value = std::string(t.substr(1));
    value += '\n';
    return value;
  }
  t = t.substr(1, t.size() - 2);
  if (t.find('\\') == std::string_view::npos) return std::string(t);
  std::string out;
  out.reserve(t.size());
  const auto hex = [](char c) {
    return c <= '9' ? c - '0' : c <= 'F' ? c - 'A' + 10 : c - 'a' + 10;
  };
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t[i] != '\\') {
      out += t[i];
      continue;
    }
    char c = t[++i];
    if (c == 'x') {
      const auto first = hex(t[++i]);
      const auto second = hex(t[++i]);
      out += char(first * 16 + second);
    } else if (c >= '0' && c <= '7') {
      unsigned v = c - '0';
      for (unsigned j = 0;
           j < 2 && i + 1 < t.size() && t[i + 1] >= '0' && t[i + 1] <= '7'; ++j)
        v = v * 8 + (t[++i] - '0');
      out += char(v);
    } else {
      switch (c) {
        case 'a':
          c = '\a';
          break;
        case 'b':
          c = '\b';
          break;
        case 'f':
          c = '\f';
          break;
        case 'n':
          c = '\n';
          break;
        case 'r':
          c = '\r';
          break;
        case 't':
          c = '\t';
          break;
        case 'v':
          c = '\v';
          break;
      }
      out += c;
    }
  }
  return out;
};
inline constexpr auto join_text = [](cursiv::Parts parts) {
  std::size_t size = 0;
  for (auto p : parts) size += p.size();
  std::string out;
  out.reserve(size);
  for (auto p : parts) out += p;
  return out;
};
inline constexpr auto decode_bytes = [](std::string_view t) {
  t = t.substr(3, t.size() - 4);
  std::string out;
  out.reserve(t.size() / 2);
  int first = -1;
  for (char c : t) {
    const int digit = c >= '0' && c <= '9'   ? c - '0'
                      : c >= 'a' && c <= 'f' ? c - 'a' + 10
                      : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                             : -1;
    if (digit < 0) continue;
    if (first < 0)
      first = digit;
    else {
      out += char(first * 16 + digit);
      first = -1;
    }
  }
  return out;
};
}  // namespace capnproto
