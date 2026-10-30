// The maml language: its grammar.
#pragma once

#include <string_view>
#include <charconv>
#include <cstdint>
#include <memory_resource>
#include <variant>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "maml/core/ast.hpp"

namespace maml {

inline cursiv::ConversionResult<std::string_view> string_value(
    std::string_view text, std::pmr::memory_resource* memory) {
  text = text.substr(1, text.size() - 2);
  if (text.find('\\') == text.npos)
    return cursiv::ConversionResult<std::string_view>::ok(text);
  const auto room = cursiv::text_room(memory, text.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '\\') { room[n++] = text[i]; continue; }
    const char escaped = text[++i];
    switch (escaped) {
      case 'n': room[n++] = '\n'; break;
      case 'r': room[n++] = '\r'; break;
      case 't': room[n++] = '\t'; break;
      case '"': room[n++] = '"'; break;
      case '\\': room[n++] = '\\'; break;
      case 'u': {
        const auto start = i + 2;
        const auto end = text.find('}', start);
        std::uint32_t cp = 0;
        const auto converted = std::from_chars(text.data() + start, text.data() + end, cp, 16);
        if (converted.ec != std::errc{} || cp > 0x10ffff ||
            (cp >= 0xd800 && cp <= 0xdfff))
          return cursiv::ConversionResult<std::string_view>::err("Expected a Unicode scalar value.");
        i = end;
        if (cp < 0x80) room[n++] = static_cast<char>(cp);
        else {
          if (cp >= 0x10000) room[n++] = static_cast<char>(0xf0 | (cp >> 18));
          else if (cp >= 0x800) room[n++] = static_cast<char>(0xe0 | (cp >> 12));
          else room[n++] = static_cast<char>(0xc0 | (cp >> 6));
          if (cp >= 0x10000) room[n++] = static_cast<char>(0x80 | ((cp >> 12) & 63));
          if (cp >= 0x800) room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 63));
          room[n++] = static_cast<char>(0x80 | (cp & 63));
        }
        break;
      }
    }
  }
  return cursiv::ConversionResult<std::string_view>::ok(std::string_view(room.data(), n));
}
inline constexpr auto raw_value = [](std::string_view text) {
  text = text.substr(3, text.size() - 6);
  if (text.starts_with("\r\n")) text.remove_prefix(2);
  else if (text.starts_with('\n')) text.remove_prefix(1);
  return text;
};
inline constexpr auto null_value = [](std::string_view) { return std::monostate{}; };
struct Maml : cursiv::GrammarSpec {
  // IDENTIFIER admits '-', past what the default word class covers: without
  // this, kw<"true"> would read only "true" out of "true-ish" and leave
  // "-ish" behind instead of losing to the longer identifier.
  static constexpr auto word_boundary = cr<"A-Za-z0-9_-">;

  struct WS : terminal<some(cr<R"( \t)">)> {};

  // The terminating newline is left for NL, which still acts as a separator.
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n\x00-\x08\x0a-\x1f\x7f)">)> {};

  struct NL : terminal<some(option(lit<"\r">) + lit<"\n">)> {};

  struct TRUE : terminal<as<bool>(kw<"true">)> {};
  struct FALSE : terminal<as<bool>(kw<"false">)> {};
  struct NULLK : terminal<as<null_value>(kw<"null">)> {};

  // Raw strings are bounded by """ and preserve their contents literally, at
  // least one character of them -- empty raw strings are spelled "" instead.
  // Checked before STRING so the delimiter is never read as "" plus a quote.
  struct RAW_STRING
      : terminal<as<raw_value>(through(lit<"\"\"\""> + !lit<"\"\"\""> + any, lit<"\"\"\"">))> {
  };

  static constexpr auto hexd = cr<"0-9a-fA-F">;
  static constexpr auto escape =
      lit<"\\"> +
      (cr<R"("\\tnr)"> | (lit<"u{"> + repeat<1, 6>(hexd) + lit<"}">));
  struct STRING : terminal<as<string_value>(
                      lit<"\""> + many(escape | cr<R"(^"\\\x00-\b\n-\x1f\x7f)">) +
                      lit<"\"">)> {};

  static constexpr auto integer = option(lit<"-">) +
      (lit<"0"> | (cr<"1-9"> + many(d)));
  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct INTEGER : terminal<as<std::int64_t>(integer)> {};
  struct FLOAT : terminal<as<double>(integer +
      ((lit<"."> + some(d) + option(exponent)) | exponent))> {};

  // Identifier keys: only A-Z, a-z, 0-9, _ and -.
  struct IDENTIFIER : terminal<some(cr<"A-Za-z0-9_-">)> {};

  struct Key : rule<ref<IDENTIFIER> | ref<STRING>> {};

  // A comma can follow significant newlines; it still denotes one separator.
  static constexpr auto separator =
      (many(ref<NL>) + lit<","> + many(ref<NL>)) | some(ref<NL>);

  struct Object;
  struct Array;
  static constexpr auto value =
      ref<Object> | ref<Array> | ref<STRING> | ref<RAW_STRING> |
      ref<FLOAT> | ref<INTEGER> | ref<TRUE> | ref<FALSE> | ref<NULLK>;

  struct Member
      : rule<assign<&ast::Member::key>(ref<Key>) + many(ref<NL>) +
                 lit<":"> + many(ref<NL>) +
                 assign<&ast::Member::value>(value)> {};

  struct Object
      : rule<lit<"{"> + many(ref<NL>) +
                 option(some(append<&ast::Object::members>(ref<Member>),
                             separator) +
                        option(separator)) +
                 lit<"}">> {};

  struct Array
      : rule<lit<"["> + many(ref<NL>) +
                 option(some(append<&ast::Array::items>(value), separator) +
                        option(separator)) +
                 lit<"]">> {};

  struct Doc
      : rule<many(ref<NL>) + assign<&ast::Doc::value>(value) +
                 many(ref<NL>)> {};

  using entry = Doc;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace maml
