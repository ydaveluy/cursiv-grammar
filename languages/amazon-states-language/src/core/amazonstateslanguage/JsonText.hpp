#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "cursiv/grammar/GrammarSpec.hpp"

namespace amazonstateslanguage {

// The grammar checks the escape spelling; this converts its JSON value.
inline auto json_string_value(std::string_view text) {
  using Result = cursiv::ConversionResult<std::string>;
  const auto body = text.substr(1, text.size() - 2);
  if (body.find('\\') == std::string_view::npos) return Result::ok(std::string(body));
  std::string out;
  out.reserve(body.size());
  const auto unit = [&](std::size_t offset) {
    std::uint32_t value = 0;
    for (std::size_t j = 0; j < 4; ++j) {
      const auto c = body[offset + j];
      value = value * 16 + (c <= '9' ? c - '0' : (c | 32) - 'a' + 10);
    }
    return value;
  };
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] != '\\') {
      out += body[i];
      continue;
    }
    const auto escaped = body[++i];
    if (escaped != 'u') {
      switch (escaped) {
        case 'b': out += '\b'; break;
        case 'f': out += '\f'; break;
        case 'n': out += '\n'; break;
        case 'r': out += '\r'; break;
        case 't': out += '\t'; break;
        default: out += escaped; break;
      }
      continue;
    }
    auto cp = unit(i + 1);
    i += 4;
    if (cp >= 0xd800 && cp <= 0xdbff) {
      if (i + 6 >= body.size() || body.substr(i + 1, 2) != "\\u")
        return Result::err("A high surrogate requires a low surrogate.");
      const auto low = unit(i + 3);
      if (low < 0xdc00 || low > 0xdfff)
        return Result::err("Invalid low surrogate.");
      cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
      i += 6;
    } else if (cp >= 0xdc00 && cp <= 0xdfff) {
      return Result::err("A low surrogate requires a high surrogate.");
    }
    if (cp < 0x80) out += static_cast<char>(cp);
    else if (cp < 0x800) {
      out += static_cast<char>(0xc0 | (cp >> 6));
      out += static_cast<char>(0x80 | (cp & 63));
    } else if (cp < 0x10000) {
      out += static_cast<char>(0xe0 | (cp >> 12));
      out += static_cast<char>(0x80 | ((cp >> 6) & 63));
      out += static_cast<char>(0x80 | (cp & 63));
    } else {
      out += static_cast<char>(0xf0 | (cp >> 18));
      out += static_cast<char>(0x80 | ((cp >> 12) & 63));
      out += static_cast<char>(0x80 | ((cp >> 6) & 63));
      out += static_cast<char>(0x80 | (cp & 63));
    }
  }
  return Result::ok(std::move(out));
}

// Fixed ASL names are JSON strings too: "Type" and "T\u0079pe" name
// the same field. Spell each ASCII character once, with its JSON escape.
template <cursiv::str Name, std::size_t I = 0>
constexpr auto json_name_tail() {
  if constexpr (I == Name.size()) return cursiv::lit<"\"">;
  else {
    constexpr auto plain = [] {
      const char text[] = {Name[I], '\0'};
      return cursiv::str{text};
    }();
    constexpr auto hex = [] {
      constexpr auto digits = "0123456789abcdef";
      const char text[] = {'0', '0', digits[Name[I] >> 4],
                           digits[Name[I] & 15], '\0'};
      return cursiv::str{text};
    }();
    constexpr auto hex_upper = [] {
      constexpr auto digits = "0123456789ABCDEF";
      const char text[] = {'0', '0', digits[Name[I] >> 4],
                           digits[Name[I] & 15], '\0'};
      return cursiv::str{text};
    }();
    return (cursiv::lit<plain> |
            (cursiv::lit<"\\u"> + (cursiv::lit<hex> | cursiv::lit<hex_upper>))) +
           json_name_tail<Name, I + 1>();
  }
}

template <cursiv::str Name>
inline constexpr auto json_name =
    cursiv::as<json_string_value>(cursiv::lit<"\""> + json_name_tail<Name>());

}  // namespace amazonstateslanguage
