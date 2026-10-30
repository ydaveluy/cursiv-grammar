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
      if (value < 0x80) room[n++] = static_cast<char>(value);
      else {
        room[n++] = static_cast<char>(0xc0 | (value >> 6));
        room[n++] = static_cast<char>(0x80 | (value & 63));
      }
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
  using Result = cursiv::ConversionResult<std::string_view>;
  const auto body = t.substr(1, t.size() - 2);
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] != '\\') continue;
    const char c = body[++i];
    if (c < '0' || c > '7') continue;
    unsigned value = c - '0';
    for (unsigned j = 1; j < 3 && i + 1 < body.size() &&
         body[i + 1] >= '0' && body[i + 1] <= '7'; ++j)
      value = value * 8 + body[++i] - '0';
    if (value > 0377) return Result::err("Octal escape must be in range [0, 377].");
  }
  return Result::ok(decode_escapes(body, mem));
};
inline constexpr auto unbackslash = [](std::string_view t) {
  return t.substr(1);
};
inline constexpr auto symbol_name = [](std::string_view name) {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (name.ends_with(':') || name.find("::") != std::string_view::npos)
    return Result::err("Invalid symbol or keyword token.");
  return Result::ok(name);
};
inline constexpr auto keyword_name = [](std::string_view text) {
  if (text.starts_with("::"))
    return cursiv::ConversionResult<std::string_view>::err("Invalid keyword token.");
  return symbol_name(text.substr(1));
};

struct Edn : cursiv::GrammarSpec {
  static constexpr auto Space =
      cr<R"( ,\r\n\t\f\v)"> | uc::Zs | uc::Zl | uc::Zp;
  static constexpr auto Stop = Space | cr<R"(()[]{}";\\)">;
  struct SYMBOL
      : terminal<as<symbol_name>(!(cr<"+-"> + d) +
                     ~(Stop | d | cr<R"(#:\\)">) + many(~Stop))> {};
  struct KEYWORD : terminal<as<keyword_name>(lit<":"> + some(~Stop))> {};

  static constexpr auto HexDigit = cr<"a-fA-F0-9">;
  static constexpr auto UnicodeEscape = lit<"u"> + repeat<4>(HexDigit);

  static constexpr auto StringEscape =
      lit<"\\"> + (cr<R"(trn\\"bf)"> | UnicodeEscape |
                   (cr<"0-7"> + repeat<0, 2>(cr<"0-7">)));
  struct STRING
      : terminal<as<unquote>(lit<"\""> + many(StringEscape | cr<R"(^"\\)">) +
                             lit<"\"">)> {};

  static constexpr auto Digits = some(d);
  static constexpr auto Sign = option(cr<"+-">);
  struct INT : terminal<Sign +
                        ((lit<"0"> + cr<"xX"> + some(HexDigit)) |
                         (repeat<1, 2>(d) + cr<"rR"> + some(cr<"0-9a-zA-Z">)) |
                         Digits) +
                        option(lit<"N">) + !~Stop> {};
  static constexpr auto Exponent = cr<"eE"> + Sign + Digits;
  struct FLOAT : terminal<Sign + Digits +
                          (lit<"M"> |
                           (((lit<"."> + many(d) + option(Exponent)) | Exponent) +
                            option(lit<"M">))) + !~Stop> {};
  struct RATIO : terminal<Sign + Digits + lit<"/"> + Digits + !~Stop> {};
  struct SYMBOLIC_NUMBER
      : terminal<lit<"##"> + (lit<"Inf"> | lit<"-Inf"> | lit<"NaN">) + !~Stop> {};

  // Any scalar but the four -- forced past byte mode by joining an (empty)
  // class of code points, so a raw multi-byte character reads whole.
  struct CHAR
      : terminal<as<unbackslash>(
            lit<"\\"> + (lit<"newline"> | lit<"return"> | lit<"space"> |
                         lit<"tab"> | lit<"backspace"> | lit<"formfeed"> |
                         (lit<"o"> + repeat<1, 3>(cr<"0-7">)) | UnicodeEscape |
                         ~cr<R"( \r\n\t)">) + !~Stop)> {};

  struct NIL : terminal<as<edn_null>(kw<"nil">)> {};
  struct BOOL : terminal<as<bool>(kw<"true"> | kw<"false">)> {};

  struct Symbol : rule<assign<&ast::Symbol::name>(ref<SYMBOL>)> {};
  struct Number
      : rule<assign<&ast::Number::text>(ref<INT> | ref<FLOAT> | ref<RATIO> |
                                        ref<SYMBOLIC_NUMBER>)> {};
  struct Character : rule<assign<&ast::Character::text>(ref<CHAR>)> {};

  struct Keyword : rule<assign<&ast::Keyword::name>(ref<KEYWORD>)> {};

  struct Tag;
  struct Discard;

  struct List;
  struct Vector;
  struct Map;
  struct Set;
  static constexpr auto undiscarded_value =
      ref<NIL> | ref<BOOL> | ref<STRING> | ref<Number> |
      ref<Character> | ref<Keyword> | ref<Symbol> | ref<Tag> |
      ref<List> | ref<Vector> | ref<Map> | ref<Set>;

  static constexpr auto value = undiscarded_value | ref<Discard>;
  struct Tag : rule<lit<"#"> + gap::none + assign<&ast::Tag::name>(ref<SYMBOL>) +
                        many(append<&ast::Tag::discards>(ref<Discard>)) +
                        assign<&ast::Tag::value>(undiscarded_value)> {};
  struct Discard
      : rule<lit<"#_"> +
             many(append<&ast::Discard::discards>(ref<Discard>)) +
             assign<&ast::Discard::value>(undiscarded_value)> {};

  struct List
      : rule<lit<"("> + many(append<&ast::List::items>(value)) + lit<")">> {};
  struct Vector
      : rule<lit<"["> + many(append<&ast::Vector::items>(value)) + lit<"]">> {};
  struct Set
      : rule<lit<"#{"> + many(append<&ast::Set::items>(value)) + lit<"}">> {};

  struct Entry
      : rule<assign<&ast::Entry::key>(undiscarded_value) +
             many(append<&ast::Entry::discards>(ref<Discard>)) +
             assign<&ast::Entry::value>(undiscarded_value)> {};
  struct Map
      : rule<option(lit<"#:"> + assign<&ast::Map::namespace_>(ref<SYMBOL>)) +
             lit<"{"> +
             many(append<&ast::Map::discards>(ref<Discard>) |
                  append<&ast::Map::entries>(ref<Entry>)) +
             lit<"}">> {};

  struct Program : rule<create<ast::Program> +
                            many(append<&ast::Program::values>(value))> {};

  struct WS : terminal<some(Space)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  using entry = Program;
  using ignored = list<WS, COMMENT>;
  static constexpr auto word_boundary = ~Stop;
};

} // namespace edn
