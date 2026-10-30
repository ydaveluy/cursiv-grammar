// The powerquery language: its grammar.
#pragma once

#include <cstdint>
#include <memory_resource>
#include <limits>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "powerquery/core/ast.hpp"

namespace powerquery {

// Strips `open` leading bytes and the trailing quote, collapsing a doubled
// quote to one. Shared by TEXT_LITERAL, QUOTED_IDENTIFIER and the verbatim
// form: only the open delimiter's length differs between them.
inline constexpr auto strip_and_undouble =
    [](std::string_view t, std::size_t open, std::pmr::memory_resource* mem) {
      const std::string_view body = t.substr(open, t.size() - open - 1);
      if (body.find('"') == std::string_view::npos &&
          body.find("#(") == std::string_view::npos)
        return body;
      const std::span<char> room = cursiv::text_room(mem, body.size());
      std::size_t n = 0;
      auto put = [&](unsigned cp) {
        if (cp < 0x80)
          room[n++] = char(cp);
        else if (cp < 0x800) {
          room[n++] = char(0xc0 | cp >> 6);
          room[n++] = char(0x80 | (cp & 63));
        } else if (cp < 0x10000) {
          room[n++] = char(0xe0 | cp >> 12);
          room[n++] = char(0x80 | (cp >> 6 & 63));
          room[n++] = char(0x80 | (cp & 63));
        } else {
          room[n++] = char(0xf0 | cp >> 18);
          room[n++] = char(0x80 | (cp >> 12 & 63));
          room[n++] = char(0x80 | (cp >> 6 & 63));
          room[n++] = char(0x80 | (cp & 63));
        }
      };
      for (std::size_t i = 0; i < body.size(); ++i) {
        if (body[i] == '#' && i + 1 < body.size() && body[i + 1] == '(') {
          const auto end = body.find(')', i + 2);
          if (end != std::string_view::npos) {
            const auto saved = n;
            bool valid = true;
            for (std::size_t pos = i + 2; pos < end;) {
              const auto comma = body.find(',', pos);
              const auto stop =
                  comma == std::string_view::npos || comma > end ? end : comma;
              const auto code = body.substr(pos, stop - pos);
              if (code == "cr")
                put('\r');
              else if (code == "lf")
                put('\n');
              else if (code == "tab")
                put('\t');
              else if (code == "#")
                put('#');
              else {
                unsigned cp = 0;
                if (code.size() != 4 && code.size() != 8) {
                  valid = false;
                  break;
                }
                for (const char c : code) {
                  const int v = c >= '0' && c <= '9'   ? c - '0'
                                : c >= 'a' && c <= 'f' ? c - 'a' + 10
                                : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                                       : -1;
                  if (v < 0) {
                    valid = false;
                    break;
                  }
                  cp = cp * 16 + unsigned(v);
                }
                if (!valid || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) {
                  valid = false;
                  break;
                }
                put(cp);
              }
              pos = stop + 1;
            }
            if (valid) {
              i = end;
              continue;
            }
            n = saved;  // Invalid escape stays in source form for validation.
          }
        }
        room[n++] = body[i];
        if (body[i] == '"' && i + 1 < body.size() && body[i + 1] == '"') ++i;
      }
      return std::string_view(room.data(), n);
    };
inline constexpr auto unquote_text =
    [](std::string_view t, std::pmr::memory_resource* mem) {
      return strip_and_undouble(t, 1, mem);
    };
inline constexpr auto unquote_hash =
    [](std::string_view t, std::pmr::memory_resource* mem) {
      return strip_and_undouble(t, 2, mem);
    };
// A verbatim literal uses #!"..." and the same text escaping as a text literal.
inline constexpr auto unquote_verbatim = [](std::string_view t,
                                            std::pmr::memory_resource* mem) {
  return strip_and_undouble(t, 3, mem);
};
// Identifiers are the same span raw, unless they took the `#"..."` form.
inline constexpr auto identifier_text =
    [](std::string_view t, std::pmr::memory_resource* mem) -> std::string_view {
  if (t.size() >= 2 && t[0] == '#' && t[1] == '"')
    return strip_and_undouble(t, 2, mem);
  return t;
};

inline constexpr auto hex_number = [](std::string_view t) {
  double v = 0;
  for (std::size_t i = 2; i < t.size(); ++i) {
    const char c = t[i];
    const int digit = (c <= '9') ? c - '0' : (c | 0x20) - 'a' + 10;
    v = v * 16 + digit;
  }
  return v;
};

struct Powerquery : cursiv::GrammarSpec {
  // Trivia: whitespace and both comment forms are discarded outright, so
  // nothing here is `hidden`.
  struct WS : terminal<some(cr<R"(\t\v\f \r\n)"> | uc::Zs | cr<"  ">)> {
  };
  struct LineComment : terminal<lit<"//"> + many(cr<R"(^\r\n\u0085\u2028\u2029)">)> {};
  struct BlockComment : terminal<through(lit<"/*">, lit<"*/">)> {};

  static constexpr auto word_boundary =
      uc::L | uc::Nl | uc::Nd | uc::Pc | uc::Mn | uc::Mc | uc::Cf | cr<"_">;

  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  // Text and quoted identifiers share #(hex), #(cr,lf), #(tab) and #(#).
  static constexpr auto CharEscape =
      lit<"#("> +
      some(repeat<8>(HexDigit) | repeat<4>(HexDigit) | lit<"cr"> | lit<"lf"> |
               lit<"tab"> | lit<"#">,
           lit<",">) +
      lit<")">;
  static constexpr auto TextChars = many(CharEscape | lit<"\"\""> | (!lit<"#("> + cr<R"(^")">));

  struct STRING
      : terminal<as<unquote_text>(lit<"\""> + TextChars + lit<"\"">)> {};
  struct VERBATIM
      : terminal<as<unquote_verbatim>(lit<"#!\""> + TextChars + lit<"\"">)> {};

  static constexpr auto IdStart = uc::L | uc::Nl | cr<"_">;
  static constexpr auto IdPart =
      uc::L | uc::Nl | uc::Nd | uc::Pc | uc::Mn | uc::Mc | uc::Cf | cr<"_">;
  static constexpr auto IdSegment = IdStart + many(IdPart);
  static constexpr auto reserved = kw<"and"> | kw<"as"> | kw<"each"> | kw<"else"> |
      kw<"error"> | kw<"false"> | kw<"if"> | kw<"in"> | kw<"is"> | kw<"let"> |
      kw<"meta"> | kw<"not"> | kw<"null"> | kw<"or"> | kw<"otherwise"> |
      kw<"section"> | kw<"shared"> | kw<"then"> | kw<"true"> | kw<"try"> | kw<"type">;
  static constexpr auto available_segment = !reserved + IdSegment;
  struct ID : terminal<as<identifier_text>(some(available_segment,lit<".">) |
      (lit<"#\""> + TextChars + lit<"\"">))> {};
  // Generalized field names admit keywords, a leading digit, and U+0020
  // spaces. A type field's leading optional is its modifier.
  static constexpr auto generalized_part = option(some(uc::Nd)) + some(IdSegment,lit<".">);
  static constexpr auto generalized_name = some(generalized_part,some(lit<" ">));
  struct GENERALIZED : terminal<generalized_name> {};
  struct SPEC_GENERALIZED : terminal<!kw<"optional"> + generalized_name> {};
  struct QUOTED_ID : terminal<as<identifier_text>(lit<"#\""> + TextChars + lit<"\"">)> {};
  struct FieldName : rule<ref<GENERALIZED> | ref<QUOTED_ID>> {};
  struct TypeFieldName : rule<ref<SPEC_GENERALIZED> | ref<QUOTED_ID>> {};
  struct HASH_NAME : terminal<kw<"#binary"> | kw<"#date"> | kw<"#datetime"> | kw<"#datetimezone"> |
      kw<"#duration"> | kw<"#sections"> | kw<"#shared"> | kw<"#table"> | kw<"#time">> {};
  struct SPECIAL_NUMBER : terminal<as<[](std::string_view t) {
    return t=="#nan" ? std::numeric_limits<double>::quiet_NaN() : std::numeric_limits<double>::infinity();
  }>(kw<"#infinity"> | kw<"#nan">)> {};

  static constexpr auto Digits = some(d);
  static constexpr auto Exp = cr<"eE"> + option(cr<"+-">) + Digits;
  struct NUMBER
      : terminal<as<double>((Digits + lit<"."> + Digits + option(Exp)) |
                            (lit<"."> + Digits + option(Exp)) |
                            (Digits + option(Exp)))> {};
  struct HEXNUMBER : terminal<as<hex_number>((lit<"0x"> | lit<"0X">) + some(HexDigit))> {};

  // Operators, each denoting its own constant rather than its spelling.
  struct AND : terminal<kw<"and">> {};

  // ANY, ANYNONNULL, ..., TEXT, TYPE: longest spelling first so `anynonnull`
  // and `datetimezone` never lex as their shorter prefixes. `TABLE` and
  // `FUNCTION` are their own terminals, not alternatives here, since a
  // table type and a function type also open on those same words: one
  // spelling for both readings, so the parser (not the lexer) picks the
  // production. `FUNCTION` is spelled `function`.
  struct TABLE_KW : terminal<kw<"table">> {};
  struct FUNCTION_KW : terminal<kw<"function">> {};
  struct PRIMITIVE_KW
      : terminal<kw<"anynonnull"> | kw<"any"> | kw<"datetimezone"> |
                     kw<"datetime"> | kw<"date"> | kw<"duration"> |
                     kw<"binary"> | kw<"list"> | kw<"logical"> |
                     kw<"none"> | kw<"number"> | kw<"record"> |
                     kw<"text"> | kw<"type">> {};

  struct Expression;

  struct LogicalLit
      : rule<create<ast::LogicalLit>(
            flag<&ast::LogicalLit::value>(kw<"true">) | kw<"false">)> {
  };
  struct NumberLit
      : rule<assign<&ast::NumberLit::value>(ref<HEXNUMBER> | ref<NUMBER> | ref<SPECIAL_NUMBER>)> {};
  struct TextLit : rule<assign<&ast::TextLit::value>(ref<STRING>)> {};
  struct NullLit : rule<create<ast::NullLit>(kw<"null">)> {};
  struct VerbatimLit
      : rule<assign<&ast::VerbatimLit::value>(ref<VERBATIM>)> {};
  struct LiteralAtom
      : rule<ref<LogicalLit> | ref<NumberLit> | ref<TextLit> |
                 ref<NullLit> | ref<VerbatimLit>> {};

  static constexpr auto PrimitiveTypeBody =
      assign<&ast::PrimitiveTypeNode::name>(
          ref<PRIMITIVE_KW> | ref<TABLE_KW> | ref<FUNCTION_KW> | kw<"null">);
  struct PrimitiveType
      : rule<create<ast::PrimitiveTypeNode>(PrimitiveTypeBody)> {};
  struct NullablePrimitiveType
      : rule<create<ast::PrimitiveTypeNode>(
            option(flag<&ast::PrimitiveTypeNode::nullable>(
                kw<"nullable">)) +
            PrimitiveTypeBody)> {};

  struct PrimaryType;
  struct TypeExpr;
  struct PostfixExpr;

  struct FieldSpec
      : rule<create<ast::FieldSpec>(
            option(flag<&ast::FieldSpec::optional>(kw<"optional">)) +
            assign<&ast::FieldSpec::name>(ref<TypeFieldName>) +
            option(lit<"="> + assign<&ast::FieldSpec::type>(ref<TypeExpr>)))> {
  };

  struct RecordType
      : rule<create<ast::RecordTypeNode>(
            lit<"["> +
            option(flag<&ast::RecordTypeNode::open>(lit<"...">) |
                       (some(append<&ast::RecordTypeNode::fields>(
                                 ref<FieldSpec>),
                             lit<",">) +
                        option(lit<","> +
                               flag<&ast::RecordTypeNode::open>(
                                   lit<"...">)))) +
            lit<"]">)> {};

  struct ListType
      : rule<create<ast::ListTypeNode>(
            lit<"{"> + assign<&ast::ListTypeNode::itemType>(ref<TypeExpr>) +
            lit<"}">)> {};

  struct ParamSpec
      : rule<create<ast::ParamSpec>(
            option(flag<&ast::ParamSpec::optional>(kw<"optional">)) +
            assign<&ast::ParamSpec::name>(ref<ID>) + kw<"as"> +
            assign<&ast::ParamSpec::type>(ref<TypeExpr>))> {};
  static constexpr auto ParamSpecListBody = some(
      append<&ast::FunctionTypeNode::params>(ref<ParamSpec>), lit<",">);
  // `function (` is printed with one space in the specification of a
  // function type in the printed grammar. The native parser also accepts
  // adjacent parentheses and other trivia here.
  struct FunctionType
      : rule<create<ast::FunctionTypeNode>(
            ref<FUNCTION_KW> + lit<"("> + option(ParamSpecListBody) +
            lit<")"> + kw<"as"> +
            assign<&ast::FunctionTypeNode::returnType>(
                ref<TypeExpr>))> {};

  struct TableType
      : rule<create<ast::TableTypeNode>(
            ref<TABLE_KW> + lit<"["> +
            many(append<&ast::TableTypeNode::rows>(ref<FieldSpec>),
                 lit<",">) +
            lit<"]">)> {};

  struct NullableType
      : rule<create<ast::NullableTypeNode>(
            kw<"nullable"> +
            assign<&ast::NullableTypeNode::inner>(ref<TypeExpr>))> {};

  struct PrimaryType
      : rule<ref<PrimitiveType> | ref<RecordType> | ref<ListType> |
                 ref<FunctionType> | ref<TableType> |
                 ref<NullableType>> {};

  struct ComputedType : rule<create<ast::ComputedTypeNode>(
      !(ref<PRIMITIVE_KW> | ref<TABLE_KW> | ref<FUNCTION_KW> | kw<"nullable"> | kw<"null"> | lit<"["> | lit<"{">) +
      assign<&ast::ComputedTypeNode::expr>(ref<PostfixExpr>))> {};
  struct TypeExpr : rule<ref<PrimaryType> | ref<ComputedType>> {};

  struct NameExpr
      : rule<create<ast::NameExpr>(
            (option(flag<&ast::NameExpr::inclusive>(lit<"@">)) +
             assign<&ast::NameExpr::name>(ref<ID>)) |
            assign<&ast::NameExpr::name>(ref<HASH_NAME>))> {};

  struct SectionAccess
      : rule<create<ast::SectionAccessExpr>(
            assign<&ast::SectionAccessExpr::section>(ref<ID>) + lit<"!"> +
            assign<&ast::SectionAccessExpr::member>(ref<ID>))> {};

  struct NotImplemented
      : rule<create<ast::NotImplementedExpr>(lit<"...">)> {};

  struct ListItem
      : rule<create<ast::ListItem>(
            assign<&ast::ListItem::from>(ref<Expression>) +
            option(lit<".."> +
                   assign<&ast::ListItem::to>(ref<Expression>)))> {};
  struct ListExpr
      : rule<create<ast::ListExpr>(
            lit<"{"> +
            many(append<&ast::ListExpr::items>(ref<ListItem>),
                        lit<",">) +
            lit<"}">)> {};

  struct Field
      : rule<create<ast::Field>(
            assign<&ast::Field::name>(ref<FieldName>) + lit<"="> +
            assign<&ast::Field::value>(ref<Expression>))> {};
  struct RecordExpr
      : rule<create<ast::RecordExpr>(
            lit<"["> +
            many(append<&ast::RecordExpr::fields>(ref<Field>),
                        lit<",">) +
            lit<"]">)> {};

  struct ImplicitField
      : rule<create<ast::FieldAccess>(
            lit<"["> + assign<&ast::FieldAccess::field>(ref<FieldName>) + lit<"]"> +
            option(flag<&ast::FieldAccess::optional>(lit<"?">)))> {};
  struct ImplicitProjection
      : rule<create<ast::Projection>(
            lit<"["> +
            some(lit<"["> + append<&ast::Projection::fields>(ref<FieldName>) +
                     lit<"]">,
                 lit<",">) +
            lit<"]"> +
            option(flag<&ast::Projection::optional>(lit<"?">)))> {};

  struct Atom
      : rule<ref<LiteralAtom> | ref<ListExpr> | ref<RecordExpr> |
                 ref<SectionAccess> | ref<NameExpr> |
                 (lit<"("> + ref<Expression> + lit<")">) |
                 ref<ImplicitProjection> | ref<ImplicitField> |
                 ref<NotImplemented>> {};

  struct PostfixExpr
      : rule<ref<Atom> +
             many(nest<ast::FieldAccess, &ast::FieldAccess::target>(
                      lit<"["> +
                      assign<&ast::FieldAccess::field>(ref<FieldName>) + lit<"]"> +
                      option(flag<&ast::FieldAccess::optional>(
                          lit<"?">)))  //
                  | nest<ast::Projection, &ast::Projection::target>(
                        lit<"["> +
                        some(lit<"["> +
                                 append<&ast::Projection::fields>(ref<FieldName>) +
                                 lit<"]">,
                             lit<",">) +
                        lit<"]"> +
                        option(flag<&ast::Projection::optional>(
                            lit<"?">)))  //
                  | nest<ast::ItemAccess, &ast::ItemAccess::target>(
                        lit<"{"> +
                        assign<&ast::ItemAccess::index>(ref<Expression>) +
                        lit<"}"> +
                        option(flag<&ast::ItemAccess::optional>(
                            lit<"?">)))  //
                  | nest<ast::Invoke, &ast::Invoke::callee>(
                        lit<"("> +
                        many(
                            append<&ast::Invoke::args>(ref<Expression>),
                            lit<",">) +
                        lit<")">))> {};

  struct TypeValue
      : rule<create<ast::TypeValueExpr>(
            kw<"type"> +
            assign<&ast::TypeValueExpr::type>(ref<PrimaryType>))> {};

  struct TypeExpression : rule<ref<TypeValue> | ref<PostfixExpr>> {};

  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+">) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>))  //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(lit<"-">) +
                   assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>))  //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(kw<"not">) +
                   assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>))  //
             | ref<TypeExpression>> {};

  static constexpr auto OpMeta = as<ast::Op::meta>(kw<"meta">);
  static constexpr auto OpMul = as<ast::Op::multiply>(lit<"*">) | as<ast::Op::divide>(lit<"/">);
  static constexpr auto OpAdd = as<ast::Op::add>(lit<"+">) | as<ast::Op::subtract>(lit<"-">) | as<ast::Op::concat>(lit<"&">);
  static constexpr auto OpRel =
      as<ast::Op::less_equal>(lit<"<=">) | as<ast::Op::greater_equal>(lit<">=">) | as<ast::Op::less>(lit<"<">) | as<ast::Op::greater>(lit<">">);
  static constexpr auto OpEq = as<ast::Op::not_equal>(lit<"<>">) | as<ast::Op::equal>(lit<"=">);
  static constexpr auto OpAnd = as<ast::Op::logical_and>(ref<AND>);
  static constexpr auto OpOr = as<ast::Op::logical_or>(kw<"or">);

  // The M language specification's relational- and equality-expression
  // productions right-recurse; arithmetic associates left, and metadata
  // permits a single unparenthesized operation.
  struct TightExpr
      : infix<UnaryExpr, ast::BinaryExpr,
              levels(nonassoc(OpMeta),  //
                     left(OpMul),  //
                     left(OpAdd),  //
                     right(OpRel),  //
                     right(OpEq))> {};

  struct AsExpr
      : rule<ref<TightExpr> +
             many(nest<ast::AsExpr, &ast::AsExpr::operand>(
                 kw<"as"> +
                 assign<&ast::AsExpr::type>(ref<NullablePrimitiveType>)))> {};
  struct IsExpr
      : rule<ref<AsExpr> +
             many(nest<ast::IsExpr, &ast::IsExpr::operand>(
                 kw<"is"> +
                 assign<&ast::IsExpr::type>(ref<NullablePrimitiveType>)))> {};

  struct LogicalExpr
      : infix<IsExpr, ast::BinaryExpr, levels(right(OpAnd), right(OpOr))> {};
  struct CoalescingExpr : infix<LogicalExpr, ast::BinaryExpr, right(as<ast::Op::coalesce>(lit<"??">))> {};

  struct Param
      : rule<create<ast::Param>(
            option(flag<&ast::Param::optional>(kw<"optional">)) +
            assign<&ast::Param::name>(ref<ID>) +
            option(kw<"as"> +
                   assign<&ast::Param::type>(ref<NullablePrimitiveType>)))> {};
  static constexpr auto ParamListBody =
      some(append<&ast::FunctionExpr::params>(ref<Param>), lit<",">);
  struct FunctionExpr
      : rule<create<ast::FunctionExpr>(
            lit<"("> + option(ParamListBody) + lit<")"> +
            option(kw<"as"> + assign<&ast::FunctionExpr::returnType>(
                                    ref<NullablePrimitiveType>)) +
            lit<"=>"> + assign<&ast::FunctionExpr::body>(ref<Expression>))> {
  };

  struct EachExpr
      : rule<create<ast::EachExpr>(
            kw<"each"> + assign<&ast::EachExpr::body>(ref<Expression>))> {};

  struct Variable
      : rule<create<ast::Variable>(
            assign<&ast::Variable::name>(ref<ID>) + lit<"="> +
            assign<&ast::Variable::value>(ref<Expression>))> {};
  struct LetExpr
      : rule<create<ast::LetExpr>(
            kw<"let"> +
            some(append<&ast::LetExpr::variables>(ref<Variable>),
                 lit<",">) +
            kw<"in"> + assign<&ast::LetExpr::body>(ref<Expression>))> {};

  struct IfExpr
      : rule<create<ast::IfExpr>(
            kw<"if"> + assign<&ast::IfExpr::condition>(ref<Expression>) +
            kw<"then"> +
            assign<&ast::IfExpr::thenExpr>(ref<Expression>) + kw<"else"> +
            assign<&ast::IfExpr::elseExpr>(ref<Expression>))> {};

  struct ErrorExpr
      : rule<create<ast::ErrorExpr>(
            kw<"error"> + assign<&ast::ErrorExpr::value>(ref<Expression>))> {
  };

  struct CatchFunction : rule<create<ast::FunctionExpr>(lit<"("> +
      option(append<&ast::FunctionExpr::params>(create<ast::Param>(assign<&ast::Param::name>(ref<ID>)))) +
      lit<")"> + lit<"=>"> + assign<&ast::FunctionExpr::body>(ref<Expression>))> {};
  struct TryExpr
      : rule<create<ast::TryExpr>(
            kw<"try"> +
            assign<&ast::TryExpr::protectedExpr>(ref<Expression>) +
            option((kw<"otherwise"> + assign<&ast::TryExpr::otherwiseExpr>(ref<Expression>)) |
                   (kw<"catch"> + assign<&ast::TryExpr::catchFunction>(ref<CatchFunction>))))> {
  };

  struct Expression
      : rule<ref<CoalescingExpr> | ref<EachExpr> | ref<FunctionExpr> |
                 ref<LetExpr> | ref<IfExpr> | ref<ErrorExpr> |
                 ref<TryExpr>> {};

  // Literal-only mirror of record/list expressions, for a section or
  // member's attribute value.
  struct AnyLiteral;
  struct RecordLiteral
      : rule<create<ast::RecordExpr>(
            lit<"["> +
            many(append<&ast::RecordExpr::fields>(
                            create<ast::Field>(
                                assign<&ast::Field::name>(ref<FieldName>) +
                                lit<"="> +
                                assign<&ast::Field::value>(
                                    ref<AnyLiteral>))),
                        lit<",">) +
            lit<"]">)> {};
  struct ListLiteral
      : rule<create<ast::ListExpr>(
            lit<"{"> +
            many(
                append<&ast::ListExpr::items>(create<ast::ListItem>(
                    assign<&ast::ListItem::from>(ref<AnyLiteral>))),
                lit<",">) +
            lit<"}">)> {};
  struct AnyLiteral
      : rule<ref<RecordLiteral> | ref<ListLiteral> |
                 ref<LogicalLit> | ref<NumberLit> | ref<TextLit> | ref<NullLit>> {};

  struct SectionMember
      : rule<create<ast::SectionMember>(
            option(assign<&ast::SectionMember::attributes>(
                ref<RecordLiteral>)) +
            option(flag<&ast::SectionMember::shared>(kw<"shared">)) +
            assign<&ast::SectionMember::name>(ref<ID>) + lit<"="> +
            assign<&ast::SectionMember::value>(ref<Expression>) + lit<";">)> {
  };

  struct Section
      : rule<create<ast::Section>(
            option(assign<&ast::Section::attributes>(ref<RecordLiteral>)) +
            kw<"section"> + assign<&ast::Section::name>(ref<ID>) + lit<";"> +
            many(append<&ast::Section::members>(ref<SectionMember>)))> {};

  struct Doc
      : rule<create<ast::Doc>(
            assign<&ast::Doc::section>(ref<Section>) |
            assign<&ast::Doc::expr>(ref<Expression>))> {};

  using entry = Doc;
  using ignored = list<WS, LineComment, BlockComment>;
};

}  // namespace powerquery
