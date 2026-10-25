// The mdx language: its grammar. MDX is a query language (SELECT ... ON
// COLUMNS FROM cube), not related to Markdown.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "mdx/core/ast.hpp"

namespace mdx {

inline constexpr auto strip_ends = [](std::string_view text,
                                      std::pmr::memory_resource* memory) {
  const char end = text.back();
  text = text.substr(1, text.size() - 2);
  const auto room = cursiv::text_room(memory, text.size());
  std::size_t size = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    room[size++] = text[i];
    if (text[i] == end && i + 1 < text.size() && text[i + 1] == end) ++i;
  }
  return std::string_view(room.data(), size);
};
using StripEnds = cursiv::text_conv<strip_ends>;

struct Mdx : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_$">;

  struct WS : terminal<some(s)> {};

  static constexpr auto id_body = cr<"a-zA-Z_$"> + many(cr<"a-zA-Z0-9_$">);
  struct ID : terminal<id_body> {};

  struct QUOTED_ID
      : terminal<lit<"["> + many(lit<"]]"> | cr<"^]\r\n">) + lit<"]">,
                 StripEnds> {};

  // NUMBER handles the optional decimal part itself: a separate
  // `digit+ '.' digit*` terminal alongside it would never be reachable
  // from any parser rule, so a decimal literal would fail to parse.
  struct AXIS_NUMBER : terminal<some(d)> {};
  struct NUMBER : terminal<some(d) + option(lit<"."> + many(d)), double> {};

  struct STRING
      : terminal<(lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">) |
                     (lit<"'"> + many(lit<"''"> | cr<"^'">) + lit<"'">),
                 StripEnds> {};

  struct QUOTE : terminal<lit<"'">> {};

  struct DOT : terminal<lit<".">> {};
  struct COMMA : terminal<lit<",">> {};
  struct LPAREN : terminal<lit<"(">> {};
  struct RPAREN : terminal<lit<")">> {};
  struct LBRACE : terminal<lit<"{">> {};
  struct RBRACE : terminal<lit<"}">> {};

  struct CONCAT : terminal<lit<"||">, value<ast::BinaryOp::concat>> {};
  struct ASTERISK : terminal<lit<"*">, value<ast::BinaryOp::multiply>> {};
  struct SOLIDUS : terminal<lit<"/">, value<ast::BinaryOp::divide>> {};

  struct EQ : terminal<lit<"=">, value<ast::BinaryOp::eq>> {};
  struct NE : terminal<lit<"<>">, value<ast::BinaryOp::ne>> {};
  struct LE : terminal<lit<"<=">, value<ast::BinaryOp::le>> {};
  struct GE : terminal<lit<">=">, value<ast::BinaryOp::ge>> {};
  struct LT : terminal<lit<"<">, value<ast::BinaryOp::lt>> {};
  struct GT : terminal<lit<">">, value<ast::BinaryOp::gt>> {};
  struct COLON : terminal<lit<":">, value<ast::BinaryOp::range>> {};

  struct XOR : terminal<ikw<"XOR">, value<ast::BinaryOp::logical_xor>> {};
  struct OR : terminal<ikw<"OR">, value<ast::BinaryOp::logical_or>> {};
  struct AND : terminal<ikw<"AND">, value<ast::BinaryOp::logical_and>> {};
  struct NOT : terminal<ikw<"NOT">, value<ast::UnaryOp::logical_not>> {};

  // `+`/`-` carry a different constant as a binary operator (term2) than
  // as the unary sign (factor): two data rules over the same literal,
  // never a terminal converted two ways.
  struct PlusOp : rule<lit<"+">, value<ast::BinaryOp::add>> {};
  struct MinusOp : rule<lit<"-">, value<ast::BinaryOp::subtract>> {};
  struct UnaryPlusOp : rule<lit<"+">, value<ast::UnaryOp::plus>> {};
  struct UnaryMinusOp : rule<lit<"-">, value<ast::UnaryOp::minus>> {};

  struct AS : terminal<ikw<"AS">> {};
  struct CASE : terminal<ikw<"CASE">> {};
  struct CELL : terminal<ikw<"CELL">> {};
  struct CELL_ORDINAL : terminal<ikw<"CELL_ORDINAL">> {};
  struct DIMENSION : terminal<ikw<"DIMENSION">> {};
  struct ELSE : terminal<ikw<"ELSE">> {};
  struct EMPTY : terminal<ikw<"EMPTY">> {};
  struct END : terminal<ikw<"END">> {};
  struct FORMATTED_VALUE : terminal<ikw<"FORMATTED_VALUE">> {};
  struct FROM : terminal<ikw<"FROM">> {};
  struct MEMBER : terminal<ikw<"MEMBER">> {};
  struct NON : terminal<ikw<"NON">> {};
  struct ON : terminal<ikw<"ON">> {};
  struct PROPERTIES : terminal<ikw<"PROPERTIES">> {};
  struct SELECT : terminal<ikw<"SELECT">> {};
  struct SET : terminal<ikw<"SET">> {};
  struct THEN : terminal<ikw<"THEN">> {};
  struct VALUE : terminal<ikw<"VALUE">> {};
  struct WHEN : terminal<ikw<"WHEN">> {};
  struct WHERE : terminal<ikw<"WHERE">> {};
  struct WITH : terminal<ikw<"WITH">> {};

  // `CREATE`, `GLOBAL`, `SESSION` and `;` are dead lexer rules in the
  // reference that still reserve those spellings. Left unspelled: those
  // words are ordinary names here.

  // A name that is a keyword only where nothing else can be: `DIMENSION`
  // and `PROPERTIES` are also ordinary identifiers, spelled once and read
  // both ways by the parser state that allows it.
  static constexpr auto identifier_text =
      ref<DIMENSION> | ref<PROPERTIES> | ref<ID> | ref<QUOTED_ID>;
  struct KeyPartRule
      : rule<lit<"&"> + (flag<&ast::KeyPart::quoted>(assign<&ast::KeyPart::name>(ref<QUOTED_ID>)) |
                         assign<&ast::KeyPart::name>(ref<ID>))> {};
  struct KeyIdentifierRule
      : rule<some(append<&ast::KeyIdentifier::parts>(ref<KeyPartRule>))> {};

  struct NameRule
      : rule<some(append<&ast::Name::parts>(identifier_text), ref<DOT>)> {};

  struct Expression;
  struct ValueExpression;
  struct ValueExpressionPrimary;

  struct WhenClauseRule
      : rule<ref<WHEN> + assign<&ast::WhenClause::when>(ref<ValueExpression>) +
             ref<THEN> + assign<&ast::WhenClause::then>(ref<ValueExpression>)> {
  };

  struct CaseExprRule
      : rule<create<ast::CaseExpr>(
            ref<CASE> +
            option(assign<&ast::CaseExpr::operand>(ref<ValueExpression>)) +
            many(append<&ast::CaseExpr::whens>(ref<WhenClauseRule>)) +
            option(ref<ELSE> +
                   assign<&ast::CaseExpr::elseValue>(ref<ValueExpression>)) +
            ref<END>)> {};

  struct ValueExpressionPrimary0
      : rule<create<ast::FunctionCall>(
                 assign<&ast::FunctionCall::name>(identifier_text) +
                 ref<LPAREN> +
                 option(some(append<&ast::FunctionCall::args>(ref<Expression>),
                             ref<COMMA>)) +
                 ref<RPAREN>)  //
             |
             create<ast::Tuple>(
                 ref<LPAREN> +
                 some(append<&ast::Tuple::items>(ref<Expression>), ref<COMMA>) +
                 ref<RPAREN>)  //
             | create<ast::SetExpr>(
                   ref<LBRACE> +
                   option(some(append<&ast::SetExpr::items>(ref<Expression>),
                               ref<COMMA>)) +
                   ref<RBRACE>)   //
             | ref<CaseExprRule>  //
             | create<ast::StringLiteral>(
                   assign<&ast::StringLiteral::text>(ref<STRING>))  //
             | create<ast::NumberLiteral>(
                   assign<&ast::NumberLiteral::value>(ref<NUMBER>))  //
             | ref<KeyIdentifierRule> |
             create<ast::Identifier>(
                 flag<&ast::Identifier::quoted>(assign<&ast::Identifier::name>(ref<QUOTED_ID>)) |
                 assign<&ast::Identifier::name>(ref<DIMENSION> |
                                                ref<PROPERTIES> | ref<ID>))> {};

  struct ValueExpressionPrimary
      : rule<ref<ValueExpressionPrimary0> +
             many(nest<ast::MemberAccess, &ast::MemberAccess::object>(
                      ref<DOT> +
                      (flag<&ast::MemberAccess::quoted>(assign<&ast::MemberAccess::name>(ref<QUOTED_ID>)) |
                       assign<&ast::MemberAccess::name>(
                           ref<DIMENSION> | ref<PROPERTIES> | ref<ID>))) |
                  nest<ast::KeyAccess, &ast::KeyAccess::object>(
                      ref<DOT> + some(append<&ast::KeyAccess::parts>(
                                     ref<KeyPartRule>)))  //
                  | nest<ast::MethodCall, &ast::MethodCall::object>(
                        ref<DOT> +
                        assign<&ast::MethodCall::name>(identifier_text) +
                        ref<LPAREN> +
                        option(some(
                            append<&ast::MethodCall::args>(ref<Expression>),
                            ref<COMMA>)) +
                        ref<RPAREN>))> {};

  // factor: MINUS value_expression_primary | PLUS value_expression_primary
  //       | value_expression_primary -- the sign applies once, not stacked.
  struct FactorRule : rule<create<ast::UnaryExpr>(
                               assign<&ast::UnaryExpr::op>(ref<UnaryMinusOp>) +
                               assign<&ast::UnaryExpr::operand>(
                                   ref<ValueExpressionPrimary>))  //
                           | create<ast::UnaryExpr>(
                                 assign<&ast::UnaryExpr::op>(ref<UnaryPlusOp>) +
                                 assign<&ast::UnaryExpr::operand>(
                                     ref<ValueExpressionPrimary>))  //
                           | ref<ValueExpressionPrimary>> {};

  // Microsoft's operator precedence table (highest to lowest): `*`,`/`,
  // then `+`,`-`,`||`, then the comparisons -- tightest to loosest here too.
  struct Term3Rule : infix<FactorRule, &ast::BinaryExpr::left,
                           &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                           (left(ref<SOLIDUS> | ref<ASTERISK>) >>
                            left(ref<CONCAT> | ref<PlusOp> | ref<MinusOp>) >>
                            left(ref<EQ> | ref<NE> | ref<LT> | ref<GT> |
                                 ref<LE> | ref<GE>))> {};

  // term4: NOT term4 | term3 -- stacks, unlike the unary sign above.
  struct Term4Rule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<NOT>) +
                 assign<&ast::UnaryExpr::operand>(ref<Term4Rule>))  //
             | ref<Term3Rule>> {};

  // Microsoft's table: AND, then XOR, then OR, each its own level.
  struct ValueExpression
      : infix<Term4Rule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<AND>) >> left(ref<XOR>) >> left(ref<OR>))> {};

  // `:` (a range of members) ranks tighter than `*`/`/` on Microsoft's
  // table, so it sits beside the unary sign, not around the whole ladder.
  struct RangeRule
      : infix<FactorRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<COLON>)> {};

  // Term3Rule/Term4Rule above, rebuilt on RangeRule instead of FactorRule:
  // `:` then reaches everywhere Expression does except a CASE branch,
  // which reads ValueExpression -- built on Term4Rule, not this pair --
  // instead.
  struct RangeTerm3Rule
      : infix<RangeRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<SOLIDUS> | ref<ASTERISK>) >>
               left(ref<CONCAT> | ref<PlusOp> | ref<MinusOp>) >>
               left(ref<EQ> | ref<NE> | ref<LT> | ref<GT> | ref<LE> |
                    ref<GE>))> {};

  struct RangeTerm4Rule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<NOT>) +
                 assign<&ast::UnaryExpr::operand>(ref<RangeTerm4Rule>))  //
             | ref<RangeTerm3Rule>> {};

  struct Expression
      : infix<RangeTerm4Rule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<AND>) >> left(ref<XOR>) >> left(ref<OR>))> {};

  struct SetSpecRule
      : rule<create<ast::SetFormula>(
            ref<SET> + assign<&ast::SetFormula::name>(ref<NameRule>) + ref<AS> +
            ((ref<QUOTE> + assign<&ast::SetFormula::expr>(ref<Expression>) +
              ref<QUOTE>)  //
             | assign<&ast::SetFormula::expr>(ref<Expression>)))> {};

  struct MemberPropertyDefRule
      : rule<create<ast::MemberProperty>(
            assign<&ast::MemberProperty::name>(identifier_text) + ref<EQ> +
            assign<&ast::MemberProperty::value>(ref<ValueExpression>))> {};

  struct MemberSpecRule
      : rule<create<ast::MemberFormula>(
            ref<MEMBER> + assign<&ast::MemberFormula::name>(ref<NameRule>) +
            ref<AS> +
            ((ref<QUOTE> +
              assign<&ast::MemberFormula::expr>(ref<ValueExpression>) +
              ref<QUOTE>)  //
             | assign<&ast::MemberFormula::expr>(ref<ValueExpression>)) +
            ref<COMMA> +
            option(some(append<&ast::MemberFormula::properties>(
                            ref<MemberPropertyDefRule>),
                        ref<COMMA>)))> {};

  // Writes into the caller's node (AxisSpec/SelectStatement): spliced as a
  // plain expression, not its own rule<>, or the generator sees two heads.
  static constexpr auto dim_props =
      option(ref<DIMENSION>) + ref<PROPERTIES> +
      some(append<&ast::AxisSpec::dimProps>(ref<NameRule>), ref<COMMA>);

  struct AxisSpecRule
      : rule<create<ast::AxisSpec>(
            option(flag<&ast::AxisSpec::nonEmpty>(ref<NON>) + ref<EMPTY>) +
            assign<&ast::AxisSpec::expr>(ref<Expression>) + option(dim_props) +
            ref<ON> +
            assign<&ast::AxisSpec::axisName>(ref<ID> | ref<AXIS_NUMBER>))> {};

  struct CellPropertyRule
      : rule<create<ast::CellProperty>(assign<&ast::CellProperty::name>(
            ref<CELL_ORDINAL> | ref<VALUE> | ref<FORMATTED_VALUE> |
            identifier_text))> {};

  static constexpr auto cell_props =
      option(ref<CELL>) + ref<PROPERTIES> +
      some(append<&ast::SelectStatement::cellProps>(ref<CellPropertyRule>),
           ref<COMMA>);

  struct SelectStatementRule
      : rule<create<ast::SelectStatement>(
            option(ref<WITH> + some(append<&ast::SelectStatement::formulas>(
                                   ref<MemberSpecRule> | ref<SetSpecRule>))) +
            ref<SELECT> +
            option(some(append<&ast::SelectStatement::axes>(ref<AxisSpecRule>),
                        ref<COMMA>)) +
            ref<FROM> + assign<&ast::SelectStatement::cube>(ref<NameRule>) +
            option(ref<WHERE> +
                   assign<&ast::SelectStatement::slicer>(ref<Expression>)) +
            option(cell_props))> {};

  using entry = SelectStatementRule;
  using ignore = WS;
};

}  // namespace mdx
