// The ron language: its grammar.
#pragma once

#include <charconv>
#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "ron/core/ast.hpp"

namespace ron {

inline constexpr auto unquote =
    [](std::string_view t, std::pmr::memory_resource *mem) -> std::string_view {
  if (t.starts_with("r") || t.starts_with("br")) {
    const auto first = t.find('"'), last = t.rfind('"');
    return t.substr(first + 1, last - first - 1);
  }
  if (t.starts_with("b"))
    t.remove_prefix(1);
  t = t.substr(1, t.size() - 2);
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
    if (escaped == 'x') {
      room[n++] = static_cast<char>(hex(t.substr(i + 1, 2)));
      i += 2;
    } else if (escaped == 'u') {
      std::uint32_t cp = 0;
      if (i + 1 < t.size() && t[i + 1] == '{') {
        const auto end = t.find('}', i + 2);
        cp = hex(t.substr(i + 2, end - i - 2));
        i = end;
      } else {
        cp = hex(t.substr(i + 1, 4));
        i += 4;
        if (cp >= 0xd800 && cp <= 0xdbff && t.substr(i + 1, 2) == "\\u" &&
            i + 7 <= t.size()) {
          const auto low = hex(t.substr(i + 3, 4));
          if (low >= 0xdc00 && low <= 0xdfff) {
            cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
            i += 6;
          }
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
};
using Unquoted = cursiv::text_conv<unquote>;

inline constexpr auto identifier_value = [](std::string_view t) {
  return t.starts_with("r#") ? t.substr(2) : t;
};
using IdentifierConv = cursiv::text_conv<identifier_value>;

struct Ron : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n\v\f‎‏  )">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\n)">)> {};
  struct ML_COMMENT
      : terminal<lit<"/*"> +
                 many(ref<ML_COMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};

  struct ID : terminal<((uc::XID_Start | lit<"_">)+many(uc::XID_Continue)) |
                           (lit<"r#"> + some(uc::XID_Continue | cr<".+-">)),
                       IdentifierConv> {};

  static constexpr auto hex = cr<"0-9a-fA-F">;
  static constexpr auto digits = some(d | lit<"_">);
  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + digits;
  static constexpr auto escape =
      lit<"\\"> + (cr<R"(nrt0\\'"bf)"> | (lit<"x"> + repeat<2>(hex)) |
                   (lit<"u{"> + some(hex) + lit<"}">));
  static constexpr auto char_body = escape | (!lit<"'"> + !lit<"\\"> + any);
  struct NUMBER : terminal<(option(cr<"+-">) +
                            (((lit<"0"> + cr<"xX"> + some(hex | lit<"_">)) |
                              (lit<"0"> + cr<"oO"> + some(cr<"0-7_">)) |
                              (lit<"0"> + cr<"bB"> + some(cr<"01_">)) |
                              (((digits + option(lit<"."> + many(d | lit<"_">))) |
                                (lit<"."> + digits)) +
                               option(exponent)) |
                              kw<"inf"> | kw<"inff32"> | kw<"inff64"> |
                              kw<"NaN"> | kw<"NaNf32"> |
                              kw<"NaNf64">)+option((cr<"iu"> + some(d)) |
                                                   (lit<"f"> + some(d))))) |
                           (lit<"b'"> + char_body + lit<"'">)> {};
  struct STRING : terminal<lit<"\""> + many(escape | cr<R"(^"\\)">) + lit<"\"">,
                           Unquoted> {};
  struct RawTag {};
  struct RAW_STRING : terminal<through(lit<"r"> + mark<RawTag>(many(lit<"#">)) +
                                 lit<"\"">, lit<"\""> + same<RawTag>),
                               Unquoted> {};
  struct BYTE_STRING
      : terminal<lit<"b\""> + many(escape | cr<R"(^"\\)">) + lit<"\"">,
                 Unquoted> {};
  struct RawByteTag {};
  struct RAW_BYTE_STRING
      : terminal<through(lit<"br"> + mark<RawByteTag>(many(lit<"#">)) + lit<"\"">, lit<"\""> + same<RawByteTag>),
                 Unquoted> {};
  struct CHARACTER : terminal<lit<"'"> + char_body + lit<"'">, Unquoted> {};
  struct BOOLEAN : terminal<kw<"true"> | kw<"false">, bool> {};

  struct TupleRule;
  struct ListRule;
  struct StructRule;
  struct MapRule;
  struct StructItemRule;
  struct MapItemRule;

  struct CharRule : rule<create<ast::CharLit> +
                         assign<&ast::CharLit::text>(ref<CHARACTER>)> {};
  struct NumberRule
      : rule<create<ast::Number> + assign<&ast::Number::text>(ref<NUMBER>)> {};
  struct BytesRule : rule<create<ast::Bytes> +
                          assign<&ast::Bytes::text>(ref<BYTE_STRING> |
                                                    ref<RAW_BYTE_STRING>)> {};

  struct IdentRule
      : rule<create<ast::Ident> + assign<&ast::Ident::name>(ref<ID>)> {};

  static constexpr auto payloadNoIdent =
      ref<NumberRule> | ref<STRING> | ref<RAW_STRING> | ref<BytesRule> |
      ref<BOOLEAN> | ref<CharRule> | ref<TupleRule> | ref<ListRule> |
      ref<StructRule> | ref<MapRule>;
  static constexpr auto payload = payloadNoIdent | ref<IdentRule>;
  static constexpr auto payloadStart = cr<R"(a-zA-Z_0-9"'([{)">;

  // Top-level values sit side by side, so `a (…)` also reads as two values:
  // an ID followed by a payload is its classname, decided as it is read.
  struct ValueRule
      : rule<(assign<&ast::Value::classname>(ref<ID>) +
              assign<&ast::Value::payload>(payload)) |
             assign<&ast::Value::payload>(payloadNoIdent) |
             (assign<&ast::Value::payload>(ref<IdentRule>) + !(payloadStart))> {
  };

  struct TupleRule
      : rule<lit<"("> +
             many(append<&ast::Tuple::items>(ref<ValueRule>), some(lit<",">)) +
             option(lit<",">) + lit<")">> {};
  struct StructItemRule
      : rule<assign<&ast::StructItem::name>(ref<ID>) + lit<":"> +
             assign<&ast::StructItem::value>(ref<ValueRule>)> {};

  struct StructRule
      : rule<lit<"("> +
             some(append<&ast::StructLit::items>(ref<StructItemRule>),
                  some(lit<",">)) +
             option(lit<",">) + lit<")">> {};

  struct ListRule
      : rule<lit<"["> +
             many(append<&ast::ListV::items>(ref<ValueRule>), some(lit<",">)) +
             option(lit<",">) + lit<"]">> {};

  struct MapItemRule
      : rule<assign<&ast::MapItem::key>(ref<ValueRule>) + lit<":"> +
             assign<&ast::MapItem::value>(ref<ValueRule>)> {};

  struct MapRule : rule<lit<"{"> +
                        many(append<&ast::MapLit::items>(ref<MapItemRule>),
                             some(lit<",">)) +
                        option(lit<",">) + lit<"}">> {};

  struct AttributeRule : rule<lit<"#!["> + ref<ID> +
                              option(lit<"("> + many(ref<ID>, lit<",">) +
                                     option(lit<",">) + lit<")">) +
                              lit<"]">> {};

  struct DocumentRule
      : rule<create<ast::Document> +
             many(append<&ast::Document::attributes>(ref<AttributeRule>)) +
             many(append<&ast::Document::values>(ref<ValueRule>) +
                  option(some(lit<",">)))> {};

  using entry = DocumentRule;
  using ignore = WS;
  static constexpr auto word_boundary = uc::XID_Continue;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

} // namespace ron
