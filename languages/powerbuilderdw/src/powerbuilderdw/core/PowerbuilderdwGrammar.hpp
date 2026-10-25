// The powerbuilderdw language: its grammar. PowerBuilder DataWindow export
// files are all-uppercase by convention: keywords match in any case, but
// identifiers only ever spell A-Z, so word_boundary and identifier
// characters stay ASCII-uppercase.
#pragma once

#include <charconv>
#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "powerbuilderdw/core/ast.hpp"

namespace powerbuilderdw {

inline constexpr auto parse_number = [](std::string_view t) -> double {
  if (!t.empty()) {
    const char c = t.back();
    if (c == 'D' || c == 'd' || c == 'F' || c == 'f') t.remove_suffix(1);
  }
  double v = 0.0;
  std::from_chars(t.data(), t.data() + t.size(), v);
  return v;
};
using NumberValue = cursiv::text_conv<parse_number>;

inline constexpr auto true_value = [] { return true; };
inline constexpr auto false_value = [] { return false; };
using TrueValue = cursiv::text_conv<true_value>;
using FalseValue = cursiv::text_conv<false_value>;

// `~~` doubles a literal tilde, `~"` escapes an embedded quote -- the escape
// PowerBuilder needs to let RETRIEVE= carry a whole PBSELECT string.
inline constexpr auto unescape_dquoted = [](std::string_view t,
                                            std::pmr::memory_resource* mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('~') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] == '~' && i + 1 < body.size() &&
        (body[i + 1] == '~' || body[i + 1] == '"')) {
      room[n++] = body[i + 1];
      ++i;
    } else {
      room[n++] = body[i];
    }
  }
  return std::string_view(room.data(), n);
};
using DQuotedValue = cursiv::text_conv<unescape_dquoted>;

inline constexpr auto unescape_quoted = [](std::string_view t,
                                           std::pmr::memory_resource* mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('~') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] == '~' && i + 1 < body.size() && body[i + 1] == '\'') {
      room[n++] = '\'';
      ++i;
    } else {
      room[n++] = body[i];
    }
  }
  return std::string_view(room.data(), n);
};
using QuotedValue = cursiv::text_conv<unescape_quoted>;

// `ARGUMENTS=((...))` is captured as one opaque lexeme, up to the first
// `))`, rather than parsed; this converter keeps only the run between the
// double parens.
inline constexpr auto strip_args = [](std::string_view t) {
  return t.substr(12, t.size() - 14);
};
using ArgsValue = cursiv::text_conv<strip_args>;

struct Powerbuilderdw : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"A-Z0-9$#%_-">;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct LINE_CONTINUATION
      : terminal<lit<"&"> + many(cr<R"( \t)">) + eol> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  static constexpr auto IdCont = cr<"A-Z0-9$#%_-">;
  struct ID : terminal<cr<"A-Z"> + many(IdCont)> {};
  struct ENUM : terminal<cr<"A-Z"> + many(IdCont) + lit<"!">> {};

  static constexpr auto NumCore = (some(d) + lit<"."> + some(d)) |
                                  (lit<"."> + some(d)) | some(d);
  struct NUMBER
      : terminal<NumCore +
                     option(cr<"Ee"> + option(cr<"+-">) + some(d)) +
                     option(cr<"DFdf">),
                 NumberValue> {};

  struct DATE : terminal<repeat<4>(d) + lit<"-"> + repeat<2>(d) + lit<"-"> +
                         repeat<2>(d)> {};
  struct TIME : terminal<repeat<2>(d) + lit<":"> + repeat<2>(d) + lit<":"> +
                         repeat<2>(d) +
                         option(lit<":"> + repeat<6>(d))> {};

  struct EXPORT_HEADER
      : terminal<lit<"$"> + cr<"A-Z"> + many(cr<"A-Z0-9#%_-">) + lit<"$"> +
                     some(cr<"A-Z0-9. _">) + cr<R"(^\r\n)">> {};

  struct DQUOTED_STRING
      : terminal<lit<"\""> +
                     many(lit<"~~"> | lit<"~\""> | cr<R"(^")">) + lit<"\"">,
                 DQuotedValue> {};
  struct QUOTED_STRING
      : terminal<lit<"'"> + many(lit<"~'"> | cr<"^'">) + lit<"'">,
                 QuotedValue> {};

  struct TRUE_KW : terminal<ikw<"TRUE">, TrueValue> {};
  struct FALSE_KW : terminal<ikw<"FALSE">, FalseValue> {};

  // `ARGUMENTS=((` then everything up to the closing `))`: one lexeme, a
  // literal opener the parser cannot see inside of.
  struct ArgsLit
      : terminal<through(ikw<"ARGUMENTS"> + lit<"=((">, lit<"))">), ArgsValue> {};

  static constexpr auto DataTypeKw =
      ikw<"ANY"> | ikw<"BLOB"> | ikw<"BOOLEAN"> | ikw<"BYTE"> |
      ikw<"CHARACTER"> | ikw<"CHAR"> | ikw<"DATE"> | ikw<"DATETIME"> |
      ikw<"DECIMAL"> | ikw<"DEC"> | ikw<"DOUBLE"> | ikw<"INTEGER"> |
      ikw<"INT"> | ikw<"LONG"> | ikw<"LONGLONG"> | ikw<"REAL"> |
      ikw<"STRING"> | ikw<"TIME"> | ikw<"UNSIGNEDINTEGER"> | ikw<"UINT"> |
      ikw<"UNSIGNEDLONG"> | ikw<"ULONG"> | ikw<"WINDOW">;

  // `x::name` may end on any of these words -- IdentifierNameEx admits the
  // digit-shaped TIME token here rather than requiring the TIME_TYPE
  // keyword, so `x::12:34:56` is accepted as written.
  static constexpr auto IdentifierNameEx =
      ref<ID> | ikw<"SELECT"> | ikw<"TYPE"> | ikw<"UPDATE"> |
      ikw<"DELETE"> | ikw<"OPEN"> | ikw<"CLOSE"> | ikw<"GOTO"> |
      ikw<"INSERT"> | ikw<"DESCRIBE"> | ref<TIME> | ikw<"READONLY">;

  struct Identifier
      : rule<ref<ID> |
             (ikw<"SUPER"> + lit<"::"> +
              (ikw<"CREATE"> | ikw<"DESTROY"> | IdentifierNameEx)) |
             (ref<ID> + lit<"::"> + (ikw<"CREATE"> | ikw<"DESTROY">)) |
             (ref<ID> + lit<"."> + (ikw<"CREATE"> | ikw<"DESTROY">)) |
             (ref<ID> + lit<"::"> + IdentifierNameEx)> {};

  static constexpr auto AttrNameHead = ref<ID> | ikw<"TYPE"> | ikw<"UPDATE">;
  static constexpr auto AttrNameTail =
      ref<ID> | ikw<"CASE"> | ikw<"TYPE"> | ikw<"ON"> | ikw<"DYNAMIC">;
  struct AttrName
      : rule<AttrNameHead + option(ref<NUMBER>) +
             many(lit<"."> + AttrNameTail)> {};

  struct SignedNumber : rule<option(lit<"-">) + ref<NUMBER>, NumberValue> {};

  struct SignedNumberText
      : rule<option(lit<"+"> | lit<"-">) + ref<NUMBER>> {};

  struct ArrayBound
      : rule<assign<&ast::ArrayBound::from>(ref<SignedNumberText>) +
             option(ikw<"TO"> +
                    assign<&ast::ArrayBound::to>(ref<SignedNumberText>))> {};

  struct ArrayDeclRule
      : rule<create<ast::ArrayDecl> +
             (lit<"[]"> |
              (lit<"["> +
               option(some(append<&ast::ArrayDecl::bounds>(ref<ArrayBound>),
                          lit<",">)) +
               lit<"]">))> {};

  struct TypeSpecRule
      : rule<create<ast::TypeSpec>(
            assign<&ast::TypeSpec::name>(DataTypeKw) +
            option(lit<"("> + assign<&ast::TypeSpec::size>(ref<NUMBER>) +
                   lit<")">))> {};

  struct ExpressionRule;
  struct ArgumentRule
      : rule<create<ast::Argument>(
            option(flag<&ast::Argument::byRef>(ikw<"REF">)) +
            assign<&ast::Argument::value>(ref<ExpressionRule>))> {};

  struct ExpressionRule
      : rule<create<ast::CloseCall>(
                 (ikw<"CLOSE"> + lit<"("> +
                  some(append<&ast::CloseCall::args>(ref<ArgumentRule>),
                       lit<",">) +
                  lit<")">) |
                 flag<&ast::CloseCall::isHalt>(ikw<"HALT"> +
                                                    ikw<"CLOSE">)) |
             create<ast::BraceExpr>(lit<"{">)> {};

  struct FuncCallRule
      : rule<create<ast::FuncCall>(
            assign<&ast::FuncCall::name>(ref<Identifier> | ikw<"DESCRIBE">) +
            lit<"("> +
            option(some(append<&ast::FuncCall::args>(ref<ArgumentRule>),
                       lit<",">)) +
            lit<")">)> {};

  struct ValueGroupRule
      : rule<create<ast::ValueGroup>(
            lit<"("> +
            append<&ast::ValueGroup::items>(ref<ExpressionRule> |
                                            DataTypeKw) +
            option(lit<","> +
                   append<&ast::ValueGroup::items>(ref<ExpressionRule> |
                                                   DataTypeKw)) +
            lit<")">)> {};

  struct NestedValueRule
      : rule<create<ast::NestedValue>(
            lit<"("> +
            append<&ast::NestedValue::groups>(ref<ValueGroupRule>) +
            many(lit<","> +
                append<&ast::NestedValue::groups>(ref<ValueGroupRule>)) +
            lit<")">)> {};

  static constexpr auto AttrValueExpr =
      ref<FuncCallRule> | ref<Identifier> | ref<SignedNumber> |
      ref<TRUE_KW> | ref<FALSE_KW> | ref<ENUM> | ref<DQUOTED_STRING> |
      ref<QUOTED_STRING> | ref<DATE> | ref<TIME> | ikw<"TYPE"> |
      ikw<"TO"> | ikw<"FROM"> | ikw<"REF"> | ikw<"NULL"> | ikw<"OPEN"> |
      ref<NestedValueRule> | ref<TypeSpecRule>;

  struct AttrRule;
  struct AttrRule
      : rule<(create<ast::LiteralAttr>(assign<&ast::LiteralAttr::value>(
                  ikw<"NULL"> | ref<NUMBER> | ref<DQUOTED_STRING> |
                  ref<DATE> | ref<TIME>)) |
             create<ast::NestedAttr>(
                 assign<&ast::NestedAttr::key>(ref<AttrName>) + lit<"="> +
                 lit<"("> +
                 some(append<&ast::NestedAttr::items>(ref<AttrRule>)) +
                 lit<")">) |
             create<ast::SimpleAttr>(
                 assign<&ast::SimpleAttr::key>(ref<AttrName>) + lit<"="> +
                 assign<&ast::SimpleAttr::value>(AttrValueExpr) +
                 option(assign<&ast::SimpleAttr::array>(
                     ref<ArrayDeclRule>)))) +
             option(lit<",">)> {};

  struct ColumnTypeAttrRule
      : rule<create<ast::ColumnTypeAttr>(
            ikw<"COLUMN"> + lit<"="> + lit<"("> + ikw<"TYPE"> + lit<"="> +
            assign<&ast::ColumnTypeAttr::dataType>(DataTypeKw) + lit<"("> +
            assign<&ast::ColumnTypeAttr::size>(ref<NUMBER>) + lit<")"> +
            some(append<&ast::ColumnTypeAttr::attrs>(ref<AttrRule>)) +
            lit<")">)> {};

  struct RetrieveAttrRule
      : rule<create<ast::RetrieveAttr>(
            ikw<"RETRIEVE"> + lit<"="> +
            assign<&ast::RetrieveAttr::retrieveText>(ref<DQUOTED_STRING>) +
            option(assign<&ast::RetrieveAttr::argsText>(ref<ArgsLit>)) +
            option(ikw<"SORT"> + lit<"="> +
                   assign<&ast::RetrieveAttr::sortText>(
                       ref<DQUOTED_STRING>)))> {};

  struct TablePropertyRule
      : rule<create<ast::TableProperty>(
            ikw<"TABLE"> + lit<"("> +
            some(append<&ast::TableProperty::items>(
                ref<ColumnTypeAttrRule> | ref<RetrieveAttrRule>)) +
            lit<")">)> {};

  struct ColumnPropertyRule
      : rule<create<ast::ColumnProperty>(
            ikw<"COLUMN"> + lit<"("> +
            many(append<&ast::ColumnProperty::attrs>(ref<AttrRule>)) +
            lit<")">)> {};

  struct AttrPropertyRule
      : rule<create<ast::AttrProperty>(
            assign<&ast::AttrProperty::name>(ref<AttrName>) + lit<"("> +
            many(append<&ast::AttrProperty::attrs>(ref<AttrRule>)) +
            lit<")">)> {};

  struct PropertyRule
      : rule<ref<TablePropertyRule> | ref<ColumnPropertyRule> |
             ref<AttrPropertyRule>> {};

  struct DocumentRule
      : rule<create<ast::Document>(
            many(append<&ast::Document::headerLines>(ref<EXPORT_HEADER>)) +
            option(ikw<"RELEASE"> +
                   assign<&ast::Document::release>(ref<NUMBER>) +
                   lit<";">) +
            some(append<&ast::Document::properties>(ref<PropertyRule>)))> {
  };

  using entry = DocumentRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT, LINE_CONTINUATION>;
};

}  // namespace powerbuilderdw
