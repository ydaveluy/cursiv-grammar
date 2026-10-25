// The json5 language: its grammar. JSON5 is JSON plus comments, single
// quotes, unquoted (identifier) keys, trailing commas, signed/hex/leading-
// or-trailing-dot numbers, and Infinity/NaN -- see https://json5.org/.
#pragma once

#include <charconv>
#include <cstdint>
#include <memory_resource>
#include <string_view>
#include <variant>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "json5/core/ast.hpp"

namespace json5 {

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
inline constexpr auto identifier_value = [](std::string_view t,
                                            std::pmr::memory_resource *mem) {
  return decode_escapes(t, mem);
};
using IdentifierConv = cursiv::text_conv<identifier_value>;

inline constexpr auto null_value = [](std::string_view) {
  return std::monostate{};
};
using NullValue = cursiv::text_conv<null_value>;

struct Json5 : cursiv::GrammarSpec {
  // JSON5 includes all Unicode space separators and its four line endings.
  struct WS : terminal<some(cr<R"( \t\n\r\v\f)"> | uc::Zs |
                            lit<"\xef\xbb\xbf"> | uc::Zl | uc::Zp)> {};

  // `\r\n`, `\r`, `\n`, U+2028 LINE SEPARATOR, U+2029 PARAGRAPH SEPARATOR.
  static constexpr auto newline5 = lit<"\r\n"> | lit<"\r"> | lit<"\n"> |
                                   lit<"\xe2\x80\xa8"> | lit<"\xe2\x80\xa9">;

  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n  )">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  static constexpr auto hexd = cr<"0-9a-fA-F">;
  static constexpr auto unicode_seq = lit<"u"> + repeat<4>(hexd);

  // A backslash escape: a line continuation, `\uXXXX`, a single-char escape,
  // `\xNN`, `\0` not followed by a digit, or -- the fallback the grammar
  // names -- any other byte, taken as itself.
  static constexpr auto escape5 =
      lit<"\\"> + (newline5 | unicode_seq | cr<R"('"\\/bfnrtv)"> |
                   (lit<"x"> + repeat<2>(hexd)) | (lit<"0"> + !d) |
                   cr<R"(^'"\\bfnrtv0-9xu\r\n)">);

  struct STRING
      : terminal<(lit<"\""> + many(escape5 | cr<R"(^"\\\r\n)">) + lit<"\"">) |
                     (lit<"'"> + many(escape5 | cr<R"(^'\\\r\n)">) + lit<"'">),
                 Unquoted> {};

  static constexpr auto identifier_start_char = uc::L | uc::Nl | cr<"$_">;
  static constexpr auto identifier_part_char =
      identifier_start_char | uc::M | uc::N | uc::Pc | cr<"‌‍">;
  static constexpr auto identifier_escape = lit<"\\"> + unicode_seq;
  static constexpr auto ident_start = identifier_start_char | identifier_escape;
  static constexpr auto ident_part = identifier_part_char | identifier_escape;
  struct IDENTIFIER : terminal<ident_start + many(ident_part), IdentifierConv> {
  };

  struct BOOL : terminal<kw<"true"> | kw<"false">, bool> {};
  struct NULLK : terminal<kw<"null">, NullValue> {};

  static constexpr auto decint = lit<"0"> | (cr<"1-9"> + many(d));
  // The exponent's digits are optional, so `1e` lexes as one NUMBER token.
  static constexpr auto expo = cr<"eE"> + option(cr<"+-">) + many(d);
  struct NUMBER
      : terminal<option(cr<"+-">) + (kw<"Infinity"> | kw<"NaN"> |
                                     (lit<"0"> + cr<"xX"> + some(hexd)) |
                                     (((decint + option(lit<"."> + many(d))) |
                                       (lit<"."> + some(d))) +
                                      option(expo))),
                 std::string_view> {};

  struct NumberRule : rule<assign<&ast::Number::text>(ref<NUMBER>)> {};
  struct ObjectRule;
  struct ArrayRule;
  // An unquoted plain identifier is a key, never a value.
  static constexpr auto value = ref<STRING> | ref<NumberRule> |
                                ref<ObjectRule> | ref<ArrayRule> | ref<BOOL> |
                                ref<NULLK>;

  // `true`/`false`/`null` and `Infinity`/`NaN` are themselves
  // identifier-shaped, so IDENTIFIER already reads them as a key.
  struct MemberRule
      : rule<assign<&ast::Member::name>(ref<STRING> | ref<IDENTIFIER>) +
             lit<":"> + assign<&ast::Member::value>(value)> {};

  struct ObjectRule
      : rule<lit<"{"> +
             option(some(append<&ast::Object::members>(ref<MemberRule>),
                         lit<",">) +
                    option(lit<",">)) +
             lit<"}">> {};

  struct ArrayRule
      : rule<lit<"["> +
             option(some(append<&ast::Array::items>(value), lit<",">) +
                    option(lit<",">)) +
             lit<"]">> {};

  // The entry rule is exempt from the "matches at least one token" rule,
  // so an empty document is legal.
  struct DocRule : rule<option(assign<&ast::Doc::value>(value))> {};

  using entry = DocRule;
  using ignore = list<WS, SL_COMMENT, ML_COMMENT>;

  // `IDENTIFIER_PART`'s own alphabet, so a keyword (`true`, `Infinity`...)
  // may not run into a name continuing past it in any of its scalars.
  static constexpr auto word_boundary = identifier_part_char | cr<R"(\\)">;
};

} // namespace json5
