#pragma once
#include <cstddef>
#include <memory_resource>
#include <string_view>
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace refal {
inline unsigned hex_value(char c) {
  if (c >= '0' && c <= '9') return static_cast<unsigned>(c-'0');
  if (c >= 'a' && c <= 'f') return static_cast<unsigned>(c-'a'+10);
  return static_cast<unsigned>(c-'A'+10);
}
inline constexpr auto decode_string =
    [](std::string_view token, std::pmr::memory_resource* mem) {
  const auto body = token.substr(1, token.size()-2);
  if (body.find('\\') == std::string_view::npos) return body;
  const auto out = cursiv::text_room(mem, body.size());
  std::size_t size = 0;
  for (std::size_t i=0; i<body.size(); ++i) {
    char c = body[i];
    if (c == '\\') {
      c = body[++i];
      if (c == 'n') c = '\n';
      else if (c == 'r') c = '\r';
      else if (c == 't') c = '\t';
      else if (c == 'x') {
        c = static_cast<char>((hex_value(body[i+1])<<4)|hex_value(body[i+2]));
        i += 2;
      }
    }
    out[size++] = c;
  }
  return std::string_view(out.data(), size);
};
} // namespace refal
