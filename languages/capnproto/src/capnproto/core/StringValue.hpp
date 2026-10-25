#pragma once
#include <cstring>
#include <string>

#include "cursiv/core/grammar/GrammarSpec.hpp"
namespace capnproto {
inline std::string_view arena_text(std::string_view value,
                                   std::pmr::memory_resource* mem) {
  const auto room = cursiv::text_room(mem, value.size());
  std::memcpy(room.data(), value.data(), value.size());
  return {room.data(), value.size()};
}
inline constexpr auto decode_text = [](std::string_view t,
                                       std::pmr::memory_resource* mem) {
  if (t.starts_with('`')) {
    auto value = std::string(t.substr(1));
    value += '\n';
    return arena_text(value, mem);
  }
  t = t.substr(1, t.size() - 2);
  if (t.find('\\') == std::string_view::npos) return t;
  const auto room = cursiv::text_room(mem, t.size());
  std::size_t n = 0;
  const auto hex = [](char c) {
    return c <= '9' ? c - '0' : c <= 'F' ? c - 'A' + 10 : c - 'a' + 10;
  };
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t[i] != '\\') {
      room[n++] = t[i];
      continue;
    }
    char c = t[++i];
    if (c == 'x') {
      const auto first = hex(t[++i]);
      const auto second = hex(t[++i]);
      room[n++] = char(first * 16 + second);
    } else if (c >= '0' && c <= '7') {
      unsigned v = c - '0';
      for (unsigned j = 0;
           j < 2 && i + 1 < t.size() && t[i + 1] >= '0' && t[i + 1] <= '7'; ++j)
        v = v * 8 + (t[++i] - '0');
      room[n++] = char(v);
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
      room[n++] = c;
    }
  }
  return std::string_view(room.data(), n);
};
inline constexpr auto join_text = [](cursiv::Parts parts,
                                     std::pmr::memory_resource* mem) {
  if (parts.size() == 1) return parts[0];
  std::size_t size = 0;
  for (auto p : parts) size += p.size();
  const auto room = cursiv::text_room(mem, size);
  std::size_t at = 0;
  for (auto p : parts) {
    std::memcpy(room.data() + at, p.data(), p.size());
    at += p.size();
  }
  return std::string_view(room.data(), size);
};
inline constexpr auto decode_bytes = [](std::string_view t,
                                        std::pmr::memory_resource* mem) {
  t = t.substr(3, t.size() - 4);
  const auto room = cursiv::text_room(mem, t.size() / 2);
  std::size_t n = 0;
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
      room[n++] = char(first * 16 + digit);
      first = -1;
    }
  }
  return std::string_view(room.data(), n);
};
}  // namespace capnproto
