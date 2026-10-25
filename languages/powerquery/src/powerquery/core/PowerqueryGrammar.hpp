// The powerquery language: its grammar.
#pragma once

#include <cstdint>
#include <memory_resource>
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
          room[n++] = char(0x80 | cp & 63);
        } else if (cp < 0x10000) {
          room[n++] = char(0xe0 | cp >> 12);
          room[n++] = char(0x80 | cp >> 6 & 63);
          room[n++] = char(0x80 | cp & 63);
        } else {
          room[n++] = char(0xf0 | cp >> 18);
          room[n++] = char(0x80 | cp >> 12 & 63);
          room[n++] = char(0x80 | cp >> 6 & 63);
          room[n++] = char(0x80 | cp & 63);
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
using TextValue = cursiv::text_conv<unquote_text>;
inline constexpr auto unquote_hash =
    [](std::string_view t, std::pmr::memory_resource* mem) {
      return strip_and_undouble(t, 2, mem);
    };
using HashValue = cursiv::text_conv<unquote_hash>;
// A verbatim literal uses #!"..." and the same text escaping as a text literal.
inline constexpr auto unquote_verbatim = [](std::string_view t,
                                            std::pmr::memory_resource* mem) {
  return strip_and_undouble(t, 3, mem);
};
using VerbatimValue = cursiv::text_conv<unquote_verbatim>;
// Identifiers are the same span raw, unless they took the `#"..."` form.
inline constexpr auto identifier_text =
    [](std::string_view t, std::pmr::memory_resource* mem) -> std::string_view {
  if (t.size() >= 2 && t[0] == '#' && t[1] == '"')
    return strip_and_undouble(t, 2, mem);
  return t;
};
using IdentifierValue = cursiv::text_conv<identifier_text>;

inline constexpr auto hex_number = [](std::string_view t) {
  double v = 0;
  for (std::size_t i = 2; i < t.size(); ++i) {
    const char c = t[i];
    const int digit = (c <= '9') ? c - '0' : (c | 0x20) - 'a' + 10;
    v = v * 16 + digit;
  }
  return v;
};
using HexValue = cursiv::text_conv<hex_number>;

struct Powerquery : cursiv::GrammarSpec {
  // Trivia: whitespace and both comment forms are discarded outright, so
  // nothing here is `hidden`.
  struct WS : terminal<some(cr<R"(\t\v\f \r\n)"> | uc::Zs | cr<"  ">)> {
  };
  struct LineComment : terminal<lit<"//"> + many(cr<R"(^\n)">)> {};
  struct BlockComment : terminal<through(lit<"/*">, lit<"*/">)> {};

  static constexpr auto word_boundary =
      uc::L | uc::Nd | uc::Pc | uc::Mn | uc::Mc | uc::Cf | cr<"_">;

  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  // Text and quoted identifiers share #(hex), #(cr,lf), #(tab) and #(#).
  static constexpr auto CharEscape =
      lit<"#("> +
      some(repeat<8>(HexDigit) | repeat<4>(HexDigit) | lit<"cr"> | lit<"lf"> |
               lit<"tab"> | lit<"#">,
           lit<",">) +
      lit<")">;
  static constexpr auto TextChars = many(CharEscape | lit<"\"\""> | cr<R"(^")">);

  struct STRING : terminal<lit<"\""> + TextChars + lit<"\"">, TextValue> {};
  struct VERBATIM
      : terminal<lit<"#!\""> + TextChars + lit<"\"">, VerbatimValue> {};

  static constexpr auto IdStart = uc::L | cr<"_">;
  static constexpr auto IdPart =
      uc::L | uc::Nd | uc::Pc | uc::Mn | uc::Mc | uc::Cf | cr<"_">;
  static constexpr auto IdSegment = IdStart + many(IdPart);
  struct ID
      : terminal<some(IdSegment, lit<".">) |
                     (lit<"#\""> + TextChars + lit<"\"">),
                 IdentifierValue> {};

  static constexpr auto Digits = some(d);
  static constexpr auto Exp = cr<"eE"> + option(cr<"+-">) + Digits;
  struct NUMBER
      : terminal<(Digits + lit<"."> + Digits + option(Exp)) |
                     (lit<"."> + Digits + option(Exp)) |
                     (Digits + option(Exp)),
                 double> {};
  struct HEXNUMBER : terminal<lit<"0x"> + some(HexDigit), HexValue> {};

  // Operators, each denoting its own constant rather than its spelling.
  struct META : terminal<ikw<"meta">, value<ast::Op::meta>> {};
  struct STAR : terminal<lit<"*">, value<ast::Op::multiply>> {};
  struct SLASH : terminal<lit<"/">, value<ast::Op::divide>> {};
  struct PLUS : terminal<lit<"+">, value<ast::Op::add>> {};
  struct MINUS : terminal<lit<"-">, value<ast::Op::subtract>> {};
  struct AMP : terminal<lit<"&">, value<ast::Op::concat>> {};
  struct LEQ : terminal<lit<"<=">, value<ast::Op::less_equal>> {};
  struct GEQ : terminal<lit<">=">, value<ast::Op::greater_equal>> {};
  struct LE : terminal<lit<"<">, value<ast::Op::less>> {};
  struct GE : terminal<lit<">">, value<ast::Op::greater>> {};
  struct NEQ : terminal<lit<"<>">, value<ast::Op::not_equal>> {};
  struct EQUALS : terminal<lit<"=">, value<ast::Op::equal>> {};
  struct AND : terminal<ikw<"and">, value<ast::Op::logical_and>> {};
  struct OR : terminal<ikw<"or">, value<ast::Op::logical_or>> {};

  // ANY, ANYNONNULL, ..., TEXT, TYPE: longest spelling first so `anynonnull`
  // and `datetimezone` never lex as their shorter prefixes. `TABLE` and
  // `FUNCTION` are their own terminals, not alternatives here, since a
  // table type and a function type also open on those same words: one
  // spelling for both readings, so the parser (not the lexer) picks the
  // production. `FUNCTION` is spelled `function`.
  struct TABLE_KW : terminal<ikw<"table">> {};
  struct FUNCTION_KW : terminal<ikw<"function">> {};
  struct PRIMITIVE_KW
      : terminal<ikw<"anynonnull"> | ikw<"any"> | ikw<"datetimezone"> |
                     ikw<"datetime"> | ikw<"date"> | ikw<"duration"> |
                     ikw<"binary"> | ikw<"list"> | ikw<"logical"> |
                     ikw<"none"> | ikw<"number"> | ikw<"record"> |
                     ikw<"text"> | ikw<"type">> {};

  struct Expression;

  struct LogicalLitRule
      : rule<create<ast::LogicalLit>(
            flag<&ast::LogicalLit::value>(ikw<"true">) | ikw<"false">)> {
  };
  struct NumberLitRule
      : rule<assign<&ast::NumberLit::value>(ref<HEXNUMBER> | ref<NUMBER>)> {};
  struct TextLitRule : rule<assign<&ast::TextLit::value>(ref<STRING>)> {};
  struct NullLitRule : rule<create<ast::NullLit>(ikw<"null">)> {};
  struct VerbatimLitRule
      : rule<assign<&ast::VerbatimLit::value>(ref<VERBATIM>)> {};
  struct LiteralAtom
      : rule<ref<LogicalLitRule> | ref<NumberLitRule> | ref<TextLitRule> |
                 ref<NullLitRule> | ref<VerbatimLitRule>> {};

  static constexpr auto PrimitiveTypeBody =
      assign<&ast::PrimitiveTypeNode::name>(
          ref<PRIMITIVE_KW> | ref<TABLE_KW> | ref<FUNCTION_KW>) |
      assign<&ast::PrimitiveTypeNode::literal>(ref<LiteralAtom>);
  struct PrimitiveTypeRule
      : rule<create<ast::PrimitiveTypeNode>(PrimitiveTypeBody)> {};
  struct NullablePrimitiveType
      : rule<create<ast::PrimitiveTypeNode>(
            option(flag<&ast::PrimitiveTypeNode::nullable>(
                ikw<"nullable">)) +
            PrimitiveTypeBody)> {};

  struct PrimaryType;
  struct TypeExpr;

  struct FieldSpecRule
      : rule<create<ast::FieldSpec>(
            option(flag<&ast::FieldSpec::optional>(ikw<"optional">)) +
            assign<&ast::FieldSpec::name>(ref<ID>) +
            option(lit<"="> + assign<&ast::FieldSpec::type>(ref<TypeExpr>)))> {
  };

  struct RecordTypeRule
      : rule<create<ast::RecordTypeNode>(
            lit<"["> +
            option(flag<&ast::RecordTypeNode::open>(lit<"...">) |
                       (some(append<&ast::RecordTypeNode::fields>(
                                 ref<FieldSpecRule>),
                             lit<",">) +
                        option(lit<","> +
                               flag<&ast::RecordTypeNode::open>(
                                   lit<"...">)))) +
            lit<"]">)> {};

  struct ListTypeRule
      : rule<create<ast::ListTypeNode>(
            lit<"{"> + assign<&ast::ListTypeNode::itemType>(ref<TypeExpr>) +
            lit<"}">)> {};

  struct ParamSpecRule
      : rule<create<ast::ParamSpec>(
            option(flag<&ast::ParamSpec::optional>(ikw<"optional">)) +
            assign<&ast::ParamSpec::name>(ref<ID>) + ikw<"as"> +
            assign<&ast::ParamSpec::type>(ref<NullablePrimitiveType>))> {};
  static constexpr auto ParamSpecListBody = some(
      append<&ast::FunctionTypeNode::params>(ref<ParamSpecRule>), lit<",">);
  // `function` and `(` need exactly one space between them to open a
  // function type; cursiv's trivia already runs between the two, so this
  // accepts any gap there instead (over-acceptance, price nil).
  struct FunctionTypeRule
      : rule<create<ast::FunctionTypeNode>(
            ref<FUNCTION_KW> + lit<"("> + option(ParamSpecListBody) +
            lit<")"> + ikw<"as"> +
            assign<&ast::FunctionTypeNode::returnType>(
                ref<NullablePrimitiveType>))> {};

  struct TableTypeRule
      : rule<create<ast::TableTypeNode>(
            ref<TABLE_KW> + lit<"["> +
            some(append<&ast::TableTypeNode::rows>(ref<FieldSpecRule>),
                 lit<",">) +
            lit<"]">)> {};

  struct NullableTypeRule
      : rule<create<ast::NullableTypeNode>(
            ikw<"nullable"> +
            assign<&ast::NullableTypeNode::inner>(ref<TypeExpr>))> {};

  struct PrimaryType
      : rule<ref<PrimitiveTypeRule> | ref<RecordTypeRule> | ref<ListTypeRule> |
                 ref<FunctionTypeRule> | ref<TableTypeRule> |
                 ref<NullableTypeRule>> {};

  struct ComputedTypeRule
      : rule<create<ast::ComputedTypeNode>(
            lit<"("> + assign<&ast::ComputedTypeNode::expr>(ref<Expression>) +
            lit<")">)> {};

  struct TypeExpr : rule<ref<ComputedTypeRule> | ref<PrimaryType>> {};

  struct NameExprRule
      : rule<create<ast::NameExpr>(
            option(flag<&ast::NameExpr::inclusive>(lit<"@">)) +
            assign<&ast::NameExpr::name>(ref<ID>))> {};

  struct SectionAccessRule
      : rule<create<ast::SectionAccessExpr>(
            assign<&ast::SectionAccessExpr::section>(ref<ID>) + lit<"!"> +
            assign<&ast::SectionAccessExpr::member>(ref<ID>))> {};

  struct NotImplementedRule
      : rule<create<ast::NotImplementedExpr>(lit<"...">)> {};

  struct ListItemRule
      : rule<create<ast::ListItem>(
            assign<&ast::ListItem::from>(ref<Expression>) +
            option(lit<".."> +
                   assign<&ast::ListItem::to>(ref<Expression>)))> {};
  struct ListExprRule
      : rule<create<ast::ListExpr>(
            lit<"{"> +
            option(some(append<&ast::ListExpr::items>(ref<ListItemRule>),
                        lit<",">)) +
            lit<"}">)> {};

  struct FieldRule
      : rule<create<ast::Field>(
            assign<&ast::Field::name>(ref<ID>) + lit<"="> +
            assign<&ast::Field::value>(ref<Expression>))> {};
  struct RecordExprRule
      : rule<create<ast::RecordExpr>(
            lit<"["> +
            option(some(append<&ast::RecordExpr::fields>(ref<FieldRule>),
                        lit<",">)) +
            lit<"]">)> {};

  struct ImplicitFieldRule
      : rule<create<ast::FieldAccess>(
            lit<"["> + assign<&ast::FieldAccess::field>(ref<ID>) + lit<"]"> +
            option(flag<&ast::FieldAccess::optional>(lit<"?">)))> {};
  struct ImplicitProjectionRule
      : rule<create<ast::Projection>(
            lit<"["> +
            some(lit<"["> + append<&ast::Projection::fields>(ref<ID>) +
                     lit<"]">,
                 lit<",">) +
            lit<"]"> +
            option(flag<&ast::Projection::optional>(lit<"?">)))> {};

  struct Atom
      : rule<ref<LiteralAtom> | ref<ListExprRule> | ref<RecordExprRule> |
                 ref<SectionAccessRule> | ref<NameExprRule> |
                 (lit<"("> + ref<Expression> + lit<")">) |
                 ref<ImplicitProjectionRule> | ref<ImplicitFieldRule> |
                 ref<NotImplementedRule>> {};

  struct PostfixExpr
      : rule<ref<Atom> +
             many(nest<ast::FieldAccess, &ast::FieldAccess::target>(
                      lit<"["> +
                      assign<&ast::FieldAccess::field>(ref<ID>) + lit<"]"> +
                      option(flag<&ast::FieldAccess::optional>(
                          lit<"?">)))  //
                  | nest<ast::Projection, &ast::Projection::target>(
                        lit<"["> +
                        some(lit<"["> +
                                 append<&ast::Projection::fields>(ref<ID>) +
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
                        option(some(
                            append<&ast::Invoke::args>(ref<Expression>),
                            lit<",">)) +
                        lit<")">))> {};

  struct TypeValueRule
      : rule<create<ast::TypeValueExpr>(
            ikw<"type"> +
            assign<&ast::TypeValueExpr::type>(ref<PrimaryType>))> {};

  struct TypeExpression : rule<ref<TypeValueRule> | ref<PostfixExpr>> {};

  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+">) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>))  //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(lit<"-">) +
                   assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>))  //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(ikw<"not">) +
                   assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>))  //
             | ref<TypeExpression>> {};

  static constexpr auto OpMeta = ref<META>;
  static constexpr auto OpMul = ref<STAR> | ref<SLASH>;
  static constexpr auto OpAdd = ref<PLUS> | ref<MINUS> | ref<AMP>;
  static constexpr auto OpRel =
      ref<LEQ> | ref<GEQ> | ref<LE> | ref<GE>;
  static constexpr auto OpEq = ref<NEQ> | ref<EQUALS>;
  static constexpr auto OpAnd = ref<AND>;
  static constexpr auto OpOr = ref<OR>;

  // The M language specification's relational- and equality-expression
  // productions right-recurse; arithmetic and metadata stay left.
  struct TightExpr
      : infix<UnaryExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpMeta) >>  //
               left(OpMul) >>  //
               left(OpAdd) >>  //
               right(OpRel) >>  //
               right(OpEq))> {};

  struct AsExpr
      : rule<ref<TightExpr> +
             many(nest<ast::AsExpr, &ast::AsExpr::operand>(
                 ikw<"as"> +
                 assign<&ast::AsExpr::type>(ref<NullablePrimitiveType>)))> {};
  struct IsExpr
      : rule<ref<AsExpr> +
             many(nest<ast::IsExpr, &ast::IsExpr::operand>(
                 ikw<"is"> +
                 assign<&ast::IsExpr::type>(ref<NullablePrimitiveType>)))> {};

  struct LogicalExpr
      : infix<IsExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, (right(OpAnd) >> right(OpOr))> {};

  struct ParamRule
      : rule<create<ast::Param>(
            option(flag<&ast::Param::optional>(ikw<"optional">)) +
            assign<&ast::Param::name>(ref<ID>) +
            option(ikw<"as"> +
                   assign<&ast::Param::type>(ref<NullablePrimitiveType>)))> {};
  static constexpr auto ParamListBody =
      some(append<&ast::FunctionExpr::params>(ref<ParamRule>), lit<",">);
  struct FunctionExprRule
      : rule<create<ast::FunctionExpr>(
            lit<"("> + option(ParamListBody) + lit<")"> +
            option(ikw<"as"> + assign<&ast::FunctionExpr::returnType>(
                                    ref<NullablePrimitiveType>)) +
            lit<"=>"> + assign<&ast::FunctionExpr::body>(ref<Expression>))> {
  };

  struct EachExprRule
      : rule<create<ast::EachExpr>(
            ikw<"each"> + assign<&ast::EachExpr::body>(ref<Expression>))> {};

  struct VariableRule
      : rule<create<ast::Variable>(
            assign<&ast::Variable::name>(ref<ID>) + lit<"="> +
            assign<&ast::Variable::value>(ref<Expression>))> {};
  struct LetExprRule
      : rule<create<ast::LetExpr>(
            ikw<"let"> +
            some(append<&ast::LetExpr::variables>(ref<VariableRule>),
                 lit<",">) +
            ikw<"in"> + assign<&ast::LetExpr::body>(ref<Expression>))> {};

  struct IfExprRule
      : rule<create<ast::IfExpr>(
            ikw<"if"> + assign<&ast::IfExpr::condition>(ref<Expression>) +
            ikw<"then"> +
            assign<&ast::IfExpr::thenExpr>(ref<Expression>) + ikw<"else"> +
            assign<&ast::IfExpr::elseExpr>(ref<Expression>))> {};

  struct ErrorExprRule
      : rule<create<ast::ErrorExpr>(
            ikw<"error"> + assign<&ast::ErrorExpr::value>(ref<Expression>))> {
  };

  struct TryExprRule
      : rule<create<ast::TryExpr>(
            ikw<"try"> +
            assign<&ast::TryExpr::protectedExpr>(ref<Expression>) +
            option(ikw<"otherwise"> +
                   assign<&ast::TryExpr::otherwiseExpr>(ref<Expression>)))> {
  };

  struct Expression
      : rule<ref<LogicalExpr> | ref<EachExprRule> | ref<FunctionExprRule> |
                 ref<LetExprRule> | ref<IfExprRule> | ref<ErrorExprRule> |
                 ref<TryExprRule>> {};

  // Literal-only mirror of record/list expressions, for a section or
  // member's attribute value.
  struct AnyLiteralRule;
  struct RecordLiteralRule
      : rule<create<ast::RecordExpr>(
            lit<"["> +
            option(some(append<&ast::RecordExpr::fields>(
                            create<ast::Field>(
                                assign<&ast::Field::name>(ref<ID>) +
                                lit<"="> +
                                assign<&ast::Field::value>(
                                    ref<AnyLiteralRule>))),
                        lit<",">)) +
            lit<"]">)> {};
  struct ListLiteralRule
      : rule<create<ast::ListExpr>(
            lit<"{"> +
            option(some(
                append<&ast::ListExpr::items>(create<ast::ListItem>(
                    assign<&ast::ListItem::from>(ref<AnyLiteralRule>))),
                lit<",">)) +
            lit<"}">)> {};
  struct AnyLiteralRule
      : rule<ref<RecordLiteralRule> | ref<ListLiteralRule> |
                 ref<LiteralAtom>> {};

  struct SectionMemberRule
      : rule<create<ast::SectionMember>(
            option(assign<&ast::SectionMember::attributes>(
                ref<RecordLiteralRule>)) +
            option(flag<&ast::SectionMember::shared>(ikw<"shared">)) +
            assign<&ast::SectionMember::name>(ref<ID>) + lit<"="> +
            assign<&ast::SectionMember::value>(ref<Expression>) + lit<";">)> {
  };

  struct SectionRule
      : rule<create<ast::Section>(
            option(assign<&ast::Section::attributes>(ref<RecordLiteralRule>)) +
            ikw<"section"> + assign<&ast::Section::name>(ref<ID>) + lit<";"> +
            many(append<&ast::Section::members>(ref<SectionMemberRule>)))> {};

  struct DocRule
      : rule<create<ast::Doc>(
            assign<&ast::Doc::section>(ref<SectionRule>) |
            assign<&ast::Doc::expr>(ref<Expression>))> {};

  using entry = DocRule;
  using ignore = list<WS, LineComment, BlockComment>;
};

}  // namespace powerquery
