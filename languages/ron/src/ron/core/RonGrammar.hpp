// The ron language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "ron/core/ast.hpp"
#include "ron/core/StringValue.hpp"

namespace ron {

inline constexpr auto identifier_value = [](std::string_view t) {
  return t.starts_with("r#") ? t.substr(2) : t;
};

struct Ron : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n\v\f‎‏  )">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\n)">)> {};
  struct ML_COMMENT
      : terminal<lit<"/*"> +
                 many(ref<ML_COMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};

  struct ID : terminal<as<identifier_value>(
                  ((uc::XID_Start | lit<"_">)+many(uc::XID_Continue)) |
                  (lit<"r#"> + some(uc::XID_Continue | cr<".+-">)))> {};

  static constexpr auto hex = cr<"0-9a-fA-F">;
  static constexpr auto digits = d + many(d | lit<"_">);
  static constexpr auto exponent =
      cr<"eE"> + option(cr<"+-">) + many(lit<"_">) + digits;
  static constexpr auto integer_suffix =
      cr<"iu"> + (lit<"128"> | lit<"64"> | lit<"32"> | lit<"16"> | lit<"8">);
  static constexpr auto float_suffix = lit<"f"> + (lit<"32"> | lit<"64">);
  static constexpr auto based_integer =
      (lit<"0x"> + hex + many(hex | lit<"_">)) |
      (lit<"0o"> + cr<"0-7"> + many(cr<"0-7_">)) |
      (lit<"0b"> + cr<"01"> + many(cr<"01_">));
  // A decimal point cannot consume the first dot of a range delimiter.
  static constexpr auto point = lit<".", !lit<".">>;
  static constexpr auto float_number =
      (digits + ((point + option(digits) + option(exponent)) | exponent)) |
      (point + digits + option(exponent));
  static constexpr auto escape =
      lit<"\\"> + (cr<R"(nrt0\\'"bf)"> | (lit<"x"> + repeat<2>(hex)) |
                   (lit<"u{"> + some(hex) + lit<"}">));
  static constexpr auto string_escape =
      escape | (lit<"\\"> + (lit<"\r\n"> | lit<"\n">) + many(cr<R"( \t\r\n)">));
  static constexpr auto char_body = escape | (!lit<"'"> + !lit<"\\"> + any);
  static constexpr auto byte_char_body =
      (lit<"\\"> + (cr<R"(nrt0\\'")"> | (lit<"x"> + repeat<2>(hex)))) |
      (!lit<"'"> + !lit<"\\"> + cr<R"(\x00-\x7f)">);
  struct NUMBER
      : terminal<(option(cr<"+-">) +
                  ((based_integer + option(integer_suffix)) |
                   (float_number + option(float_suffix)) |
                   (digits + option(integer_suffix | float_suffix)) |
                   kw<"inf"> | kw<"inff32"> | kw<"inff64"> |
                   kw<"NaN"> | kw<"NaNf32"> | kw<"NaNf64">)) |
                 (lit<"b'"> + byte_char_body + lit<"'">)> {};
  struct STRING : terminal<as<unquote>(
                      lit<"\""> + many(string_escape | cr<R"(^"\\)">) + lit<"\"">)> {};
  struct RawTag {};
  struct RAW_STRING : terminal<as<unquote>(through(
                          lit<"r"> + mark<RawTag>(many(lit<"#">)) + lit<"\"">,
                          lit<"\""> + same<RawTag>))> {};
  struct BYTE_STRING
      : terminal<as<unquote>(lit<"b\""> + many(string_escape | cr<R"(^"\\)">) +
                             lit<"\"">)> {};
  struct RawByteTag {};
  struct RAW_BYTE_STRING
      : terminal<as<unquote>(
            through(lit<"br"> + mark<RawByteTag>(many(lit<"#">)) + lit<"\"">,
                    lit<"\""> + same<RawByteTag>))> {};
  struct CHARACTER : terminal<as<unquote>(lit<"'"> + char_body + lit<"'">)> {};
  struct BOOLEAN : terminal<as<bool>(kw<"true"> | kw<"false">)> {};

  struct Tuple;
  struct List;
  struct Struct;
  struct Map;
  struct StructItem;
  struct MapItem;

  struct Char : rule<create<ast::CharLit> +
                         assign<&ast::CharLit::text>(ref<CHARACTER>)> {};
  struct Number
      : rule<create<ast::Number> + assign<&ast::Number::text>(ref<NUMBER>)> {};
  struct Range
      : rule<create<ast::Range> +
             option(assign<&ast::Range::start>(ref<Number>)) +
             ((flag<&ast::Range::inclusive>(lit<"..=">) +
               assign<&ast::Range::end>(ref<Number>)) |
              (lit<".."> + option(assign<&ast::Range::end>(ref<Number>))))> {};
  struct Bytes : rule<create<ast::Bytes> +
                          assign<&ast::Bytes::text>(ref<BYTE_STRING> |
                                                    ref<RAW_BYTE_STRING>)> {};

  struct Ident
      : rule<create<ast::Ident> + assign<&ast::Ident::name>(ref<ID>)> {};

  static constexpr auto payloadNoIdent =
      ref<Range> | ref<Number> | ref<STRING> | ref<RAW_STRING> | ref<Bytes> |
      ref<BOOLEAN> | ref<Char> | ref<Tuple> | ref<List> |
      ref<Struct> | ref<Map>;
  // Only tuple and record values may carry a leading struct/variant name.
  struct Value
      : rule<(assign<&ast::Value::classname>(ref<ID>) +
              assign<&ast::Value::payload>(ref<Tuple> | ref<Struct>)) |
             assign<&ast::Value::payload>(payloadNoIdent) |
             assign<&ast::Value::payload>(ref<Ident>)> {
  };

  struct Tuple
      : rule<lit<"("> +
             option(some(append<&ast::Tuple::items>(ref<Value>), lit<",">) +
                    option(lit<",">)) + lit<")">> {};
  struct StructItem
      : rule<assign<&ast::StructItem::name>(ref<ID>) + lit<":"> +
             assign<&ast::StructItem::value>(ref<Value>)> {};

  struct Struct
      : rule<lit<"("> +
             some(append<&ast::StructLit::items>(ref<StructItem>),
                  lit<",">) +
             option(lit<",">) + lit<")">> {};

  struct List
      : rule<lit<"["> +
             option(some(append<&ast::ListV::items>(ref<Value>), lit<",">) +
                    option(lit<",">)) + lit<"]">> {};

  struct MapItem
      : rule<assign<&ast::MapItem::key>(ref<Value>) + lit<":"> +
             assign<&ast::MapItem::value>(ref<Value>)> {};

  struct Map : rule<lit<"{"> +
                        option(some(append<&ast::MapLit::items>(ref<MapItem>),
                                    lit<",">) + option(lit<",">)) + lit<"}">> {};

  static constexpr auto extension =
      kw<"implicit_some"> | kw<"unwrap_newtypes"> |
      kw<"unwrap_variant_newtypes"> | kw<"explicit_struct_names">;
  struct Attribute
      : rule<lit<"#"> + lit<"!"> + lit<"["> +
             ((kw<"enable"> + lit<"("> + some(extension, lit<",">) +
               option(lit<",">) + lit<")">) |
              ((kw<"type"> | kw<"schema">) + lit<"="> +
               (ref<STRING> | ref<RAW_STRING>))) + lit<"]">> {};

  struct Document
      : rule<create<ast::Document> +
             many(append<&ast::Document::attributes>(ref<Attribute>)) +
             append<&ast::Document::values>(ref<Value>)> {};

  using entry = Document;
  using ignored = WS;
  static constexpr auto word_boundary = uc::XID_Continue;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

} // namespace ron
