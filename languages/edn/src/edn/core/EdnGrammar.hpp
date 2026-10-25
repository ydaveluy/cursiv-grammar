// The edn language: its grammar.
#pragma once

#include <charconv>
#include <cstdint>
#include <memory_resource>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "edn/core/ast.hpp"

namespace edn {

inline constexpr auto edn_null = [](std::string_view) {
  return std::monostate{};
};
using NilValue = cursiv::text_conv<edn_null>;

inline std::string_view decode_escapes(std::string_view t,
                                       std::pmr::memory_resource *mem) {
  if (t.find('\\') == std::string_view::npos)
    return t;
  const auto room = cursiv::text_room(mem, t.size());
  std::size_t n = 0;
  const auto hex = [](std::string_view digits) {
    std::uint32_t cp = 0;
    std::from_chars(digits.data(), digits.data() + digits.size(), cp, 16);
    return cp;
  };
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t[i] != '\\' || i + 1 == t.size()) {
      room[n++] = t[i];
      continue;
    }
    const char escaped = t[++i];
    if (escaped >= '0' && escaped <= '7') {
      unsigned value = escaped - '0';
      for (unsigned count = 1;
           count < 3 && i + 1 < t.size() && t[i + 1] >= '0' && t[i + 1] <= '7';
           ++count)
        value = value * 8 + t[++i] - '0';
      room[n++] = static_cast<char>(value);
      continue;
    }
    if (escaped == '\n' || escaped == '\r') {
      if (escaped == '\r' && i + 1 < t.size() && t[i + 1] == '\n')
        ++i;
      continue;
    }
    if (t.substr(i, 3) == "\xe2\x80\xa8" || t.substr(i, 3) == "\xe2\x80\xa9") {
      i += 2;
      continue;
    }
    if (escaped == 'x') {
      room[n++] = static_cast<char>(hex(t.substr(i + 1, 2)));
      i += 2;
    } else if (escaped == 'u') {
      std::uint32_t cp = hex(t.substr(i + 1, 4));
      i += 4;
      if (cp >= 0xd800 && cp <= 0xdbff && t.substr(i + 1, 2) == "\\u" &&
          i + 7 <= t.size()) {
        const auto low = hex(t.substr(i + 3, 4));
        if (low >= 0xdc00 && low <= 0xdfff) {
          cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
          i += 6;
        }
      }
      if (cp < 0x80)
        room[n++] = static_cast<char>(cp);
      else {
        if (cp >= 0x10000)
          room[n++] = static_cast<char>(0xf0 | (cp >> 18));
        else if (cp >= 0x800)
          room[n++] = static_cast<char>(0xe0 | (cp >> 12));
        else
          room[n++] = static_cast<char>(0xc0 | (cp >> 6));
        if (cp >= 0x10000)
          room[n++] = static_cast<char>(0x80 | ((cp >> 12) & 63));
        if (cp >= 0x800)
          room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 63));
        room[n++] = static_cast<char>(0x80 | (cp & 63));
      }
    } else {
      switch (escaped) {
      case '0':
        room[n++] = '\0';
        break;
      case 'v':
        room[n++] = '\v';
        break;
      case 'b':
        room[n++] = '\b';
        break;
      case 'f':
        room[n++] = '\f';
        break;
      case 'n':
        room[n++] = '\n';
        break;
      case 'r':
        room[n++] = '\r';
        break;
      case 't':
        room[n++] = '\t';
        break;
      default:
        room[n++] = escaped;
        break;
      }
    }
  }
  return std::string_view(room.data(), n);
}
inline constexpr auto unquote = [](std::string_view t,
                                   std::pmr::memory_resource *mem) {
  return decode_escapes(t.substr(1, t.size() - 2), mem);
};
using Unquoted = cursiv::text_conv<unquote>;
inline constexpr auto unbackslash = [](std::string_view t) {
  return t.substr(1);
};
using Unbackslashed = cursiv::text_conv<unbackslash>;

struct Edn : cursiv::GrammarSpec {
  static constexpr auto Space =
      cr<R"( ,\r\n\t\f\v)"> | uc::Zs | uc::Zl | uc::Zp;
  static constexpr auto Stop = Space | cr<R"(()[]{}";\\)">;
  struct SYMBOL
      : terminal<!(cr<"+-"> + d) + ~(Stop | d | cr<R"(#:\\)">)+many(~Stop)> {};
  struct KEYWORD : terminal<lit<":"> + some(~Stop), Unbackslashed> {};

  static constexpr auto HexDigit = cr<"a-fA-F0-9">;
  static constexpr auto UnicodeEscape = lit<"u"> + repeat<4>(HexDigit);

  static constexpr auto StringEscape =
      lit<"\\"> + (cr<R"(ctrn\\"bf)"> | UnicodeEscape |
                   (cr<"0-7"> + repeat<0, 2>(cr<"0-7">)));
  struct STRING
      : terminal<lit<"\""> + many(StringEscape | cr<R"(^"\\)">) + lit<"\"">,
                 Unquoted> {};

  static constexpr auto Digits = some(d);
  static constexpr auto Sign = option(cr<"+-">);
  struct INT : terminal<Sign +
                        ((lit<"0"> + cr<"xX"> + some(HexDigit)) |
                         (repeat<1, 2>(d) + cr<"rR"> + some(cr<"0-9a-zA-Z">)) |
                         Digits) +
                        option(lit<"N">)> {};
  static constexpr auto Exponent = cr<"eE"> + Sign + Digits;
  struct FLOAT : terminal<Sign + Digits +
                          (lit<"M"> |
                           ((lit<"."> + many(d) + option(Exponent) | Exponent) +
                            option(lit<"M">)))> {};
  struct RATIO : terminal<Sign + Digits + lit<"/"> + Digits> {};
  struct SYMBOLIC_NUMBER
      : terminal<lit<"##"> + (lit<"Inf"> | lit<"-Inf"> | lit<"NaN">)> {};

  // Any scalar but the four -- forced past byte mode by joining an (empty)
  // class of code points, so a raw multi-byte character reads whole.
  struct CHAR
      : terminal<lit<"\\"> + (lit<"newline"> | lit<"return"> | lit<"space"> |
                              lit<"tab"> | lit<"backspace"> | lit<"formfeed"> |
                              (lit<"o"> + repeat<1, 3>(cr<"0-7">)) |
                              UnicodeEscape | ~cr<R"( \r\n\t)">),
                 Unbackslashed> {};

  struct NIL : terminal<kw<"nil">, NilValue> {};
  struct BOOL : terminal<kw<"true"> | kw<"false">, bool> {};

  struct SymbolRule : rule<assign<&ast::Symbol::name>(ref<SYMBOL>)> {};
  struct NumberRule
      : rule<assign<&ast::Number::text>(ref<INT> | ref<FLOAT> | ref<RATIO> |
                                        ref<SYMBOLIC_NUMBER>)> {};
  struct CharacterRule : rule<assign<&ast::Character::text>(ref<CHAR>)> {};

  struct KeywordRule : rule<assign<&ast::Keyword::name>(ref<KEYWORD>)> {};

  struct TagRule;
  struct DiscardRule;

  struct ListRule;
  struct VectorRule;
  struct MapRule;
  struct SetRule;
  static constexpr auto undiscarded_value =
      ref<NIL> | ref<BOOL> | ref<STRING> | ref<NumberRule> |
      ref<CharacterRule> | ref<KeywordRule> | ref<SymbolRule> | ref<TagRule> |
      ref<ListRule> | ref<VectorRule> | ref<MapRule> | ref<SetRule>;

  static constexpr auto value = undiscarded_value | ref<DiscardRule>;
  struct TagRule : rule<lit<"#"> + assign<&ast::Tag::name>(ref<SYMBOL>) +
                        many(append<&ast::Tag::discards>(ref<DiscardRule>)) +
                        assign<&ast::Tag::value>(undiscarded_value)> {};
  struct DiscardRule
      : rule<lit<"#_"> +
             many(append<&ast::Discard::discards>(ref<DiscardRule>)) +
             assign<&ast::Discard::value>(undiscarded_value)> {};

  struct ListRule
      : rule<lit<"("> + many(append<&ast::List::items>(value)) + lit<")">> {};
  struct VectorRule
      : rule<lit<"["> + many(append<&ast::Vector::items>(value)) + lit<"]">> {};
  struct SetRule
      : rule<lit<"#{"> + many(append<&ast::Set::items>(value)) + lit<"}">> {};

  struct EntryRule
      : rule<assign<&ast::Entry::key>(undiscarded_value) +
             many(append<&ast::Entry::discards>(ref<DiscardRule>)) +
             assign<&ast::Entry::value>(undiscarded_value)> {};
  struct MapRule
      : rule<option(lit<"#:"> + assign<&ast::Map::namespace_>(ref<SYMBOL>)) +
             lit<"{"> +
             many(append<&ast::Map::discards>(ref<DiscardRule>) |
                  append<&ast::Map::entries>(ref<EntryRule>)) +
             lit<"}">> {};

  struct ProgramRule : rule<create<ast::Program> +
                            many(append<&ast::Program::values>(value))> {};

  struct WS : terminal<some(Space)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  using entry = ProgramRule;
  using ignore = list<WS, COMMENT>;
  static constexpr auto word_boundary = ~Stop;
};

} // namespace edn
