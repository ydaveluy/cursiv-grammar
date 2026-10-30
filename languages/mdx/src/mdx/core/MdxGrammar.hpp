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

struct Mdx : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_$">;

  struct WS : terminal<some(s)> {};
  struct LINE_COMMENT : terminal<(lit<"//"> | lit<"--">) + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<lit<"/*"> + many(ref<BLOCK_COMMENT> | (!(lit<"*/"> | lit<"/*">) + any)) + lit<"*/">> {};

  static constexpr auto id_body = cr<"a-zA-Z_$"> + many(cr<"a-zA-Z0-9_$">);
  static constexpr auto reserved = ikw<"AS"> | ikw<"CASE"> | ikw<"CELL"> | ikw<"CELL_ORDINAL"> | ikw<"ELSE"> | ikw<"EMPTY"> | ikw<"END"> | ikw<"FORMATTED_VALUE"> | ikw<"FROM"> | ikw<"MEMBER"> | ikw<"NON"> | ikw<"ON"> | ikw<"SELECT"> | ikw<"SET"> | ikw<"THEN"> | ikw<"VALUE"> | ikw<"WHEN"> | ikw<"WHERE"> | ikw<"WITH"> | ikw<"CREATE"> | ikw<"GLOBAL"> | ikw<"SESSION"> | ikw<"AND"> | ikw<"OR"> | ikw<"XOR"> | ikw<"NOT"> | ikw<"IS">;
  struct ID : terminal<!reserved + id_body> {};

  struct QUOTED_ID
      : terminal<as<strip_ends>(lit<"["> + many(lit<"]]"> | cr<"^]\r\n">) +
                                lit<"]">)> {};

  // NUMBER handles the optional decimal part itself: a separate
  // `digit+ '.' digit*` terminal alongside it would never be reachable
  // from any parser rule, so a decimal literal would fail to parse.
  struct AXIS_NUMBER : terminal<some(d)> {};
  struct NUMBER : terminal<as<double>(((some(d) + option(lit<"."> + many(d))) | (lit<"."> + some(d))) +
      option(cr<"eE"> + option(cr<"+-">) + some(d)))> {};

  struct STRING
      : terminal<as<strip_ends>(
            (lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">) |
            (lit<"'"> + many(lit<"''"> | cr<"^'">) + lit<"'">))> {};

  struct QUOTE : terminal<lit<"'">> {};

  struct DOT : terminal<lit<".">> {};
  struct COMMA : terminal<lit<",">> {};
  struct LPAREN : terminal<lit<"(">> {};
  struct RPAREN : terminal<lit<")">> {};
  struct LBRACE : terminal<lit<"{">> {};
  struct RBRACE : terminal<lit<"}">> {};

  struct EQ : terminal<as<ast::BinaryOp::eq>(lit<"=">)> {};

  struct XOR : terminal<as<ast::BinaryOp::logical_xor>(ikw<"XOR">)> {};
  struct OR : terminal<as<ast::BinaryOp::logical_or>(ikw<"OR">)> {};
  struct AND : terminal<as<ast::BinaryOp::logical_and>(ikw<"AND">)> {};
  struct NOT : terminal<as<ast::UnaryOp::logical_not>(ikw<"NOT">)> {};

  // `+`/`-` carry a different constant as a binary operator (term2) than
  // as the unary sign (factor): two data rules over the same literal,
  // never a terminal converted two ways.

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

  // This entry parses SELECT queries; DDL keywords remain reserved.

  // A name that is a keyword only where nothing else can be: `DIMENSION`
  // and `PROPERTIES` are also ordinary identifiers, spelled once and read
  // both ways by the parser state that allows it.
  static constexpr auto identifier_text =
      ref<DIMENSION> | ref<PROPERTIES> | ref<ID> | ref<QUOTED_ID>;
  static constexpr auto member_word_text = ref<DIMENSION> | ref<PROPERTIES> | ref<ID> | ref<VALUE> | ref<FORMATTED_VALUE> | ref<CELL_ORDINAL>;
  static constexpr auto member_text = member_word_text | ref<QUOTED_ID>;
  struct KeyPart
      : rule<lit<"&"> + (flag<&ast::KeyPart::quoted>(assign<&ast::KeyPart::name>(ref<QUOTED_ID>)) |
                         assign<&ast::KeyPart::name>(ref<ID>))> {};
  struct KeyIdentifier
      : rule<some(append<&ast::KeyIdentifier::parts>(ref<KeyPart>))> {};

  struct Name
      : rule<some(append<&ast::Name::parts>(identifier_text), ref<DOT>)> {};

  struct Expression;
  struct ValueExpression;
  struct ValueExpressionPrimary;
  struct SelectStatementRule;

  struct WhenClause
      : rule<ref<WHEN> + assign<&ast::WhenClause::when>(ref<ValueExpression>) +
             ref<THEN> + assign<&ast::WhenClause::then>(ref<ValueExpression>)> {
  };

  struct CaseExpr
      : rule<create<ast::CaseExpr>(
            ref<CASE> +
            option(assign<&ast::CaseExpr::operand>(ref<ValueExpression>)) +
            some(append<&ast::CaseExpr::whens>(ref<WhenClause>)) +
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
             | ref<CaseExpr>  //
             | create<ast::StringLiteral>(
                   assign<&ast::StringLiteral::text>(ref<STRING>))  //
             | create<ast::NumberLiteral>(
                   assign<&ast::NumberLiteral::value>(ref<NUMBER>))  //
             | ref<KeyIdentifier> |
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
                           member_word_text))) |
                  nest<ast::KeyAccess, &ast::KeyAccess::object>(
                      ref<DOT> + some(append<&ast::KeyAccess::parts>(
                                     ref<KeyPart>)))  //
                  | nest<ast::MethodCall, &ast::MethodCall::object>(
                        ref<DOT> +
                        assign<&ast::MethodCall::name>(member_text) +
                        ref<LPAREN> +
                        option(some(
                            append<&ast::MethodCall::args>(ref<Expression>),
                            ref<COMMA>)) +
                        ref<RPAREN>))> {};

  // factor: MINUS value_expression_primary | PLUS value_expression_primary
  //       | value_expression_primary -- the sign applies once, not stacked.
  struct Factor : rule<create<ast::UnaryExpr>(
                               assign<&ast::UnaryExpr::op>(as<ast::UnaryOp::minus>(lit<"-">)) +
                               assign<&ast::UnaryExpr::operand>(
                                   ref<ValueExpressionPrimary>))  //
                           | create<ast::UnaryExpr>(
                                 assign<&ast::UnaryExpr::op>(as<ast::UnaryOp::plus>(lit<"+">)) +
                                 assign<&ast::UnaryExpr::operand>(
                                     ref<ValueExpressionPrimary>))  //
                           | ref<ValueExpressionPrimary>> {};

  // One native precedence table serves every expression slot, including CASE.
  struct Expression : infix<Factor, ast::BinaryExpr,
      levels(left(as<ast::BinaryOp::is_>(ikw<"IS">)),
             left(as<ast::BinaryOp::range>(lit<":">)),
             left(as<ast::BinaryOp::power>(lit<"^">)),
             left(as<ast::BinaryOp::divide>(lit<"/">) | as<ast::BinaryOp::multiply>(lit<"*">)),
             left(as<ast::BinaryOp::concat>(lit<"||">) | as<ast::BinaryOp::add>(lit<"+">) | as<ast::BinaryOp::subtract>(lit<"-">)),
             left(ref<EQ> | as<ast::BinaryOp::ne>(lit<"<>">) | as<ast::BinaryOp::lt>(lit<"<">) | as<ast::BinaryOp::gt>(lit<">">) | as<ast::BinaryOp::le>(lit<"<=">) | as<ast::BinaryOp::ge>(lit<">=">)),
             prefix<ast::UnaryExpr>(ref<NOT>), left(ref<AND>), left(ref<XOR>), left(ref<OR>))> {};
  struct ValueExpression : rule<ref<Expression>> {};

  struct SetSpec
      : rule<create<ast::SetFormula>(
            ref<SET> + assign<&ast::SetFormula::name>(ref<Name>) + ref<AS> +
            ((ref<QUOTE> + assign<&ast::SetFormula::expr>(ref<Expression>) +
              ref<QUOTE>)  //
             | assign<&ast::SetFormula::expr>(ref<Expression>)))> {};

  struct MemberPropertyDef
      : rule<create<ast::MemberProperty>(
            assign<&ast::MemberProperty::name>(identifier_text) + ref<EQ> +
            assign<&ast::MemberProperty::value>(ref<ValueExpression>))> {};

  struct MemberSpec
      : rule<create<ast::MemberFormula>(
            ref<MEMBER> + assign<&ast::MemberFormula::name>(ref<Name>) +
            ref<AS> +
            ((ref<QUOTE> +
              assign<&ast::MemberFormula::expr>(ref<ValueExpression>) +
              ref<QUOTE>)  //
             | assign<&ast::MemberFormula::expr>(ref<ValueExpression>)) +
            option(ref<COMMA> + some(append<&ast::MemberFormula::properties>(
                            ref<MemberPropertyDef>), ref<COMMA>)))> {};

  // Writes into the caller's node (AxisSpec/SelectStatement): spliced as a
  // plain expression, not its own rule<>, or the generator sees two heads.
  static constexpr auto dim_props =
      option(ref<DIMENSION>) + ref<PROPERTIES> +
      some(append<&ast::AxisSpec::dimProps>(ref<Name>), ref<COMMA>);

  struct AxisSpecRule
      : rule<create<ast::AxisSpec>(
            option(flag<&ast::AxisSpec::nonEmpty>(ref<NON>) + ref<EMPTY>) +
            assign<&ast::AxisSpec::expr>(ref<Expression>) + option(dim_props) +
            ref<ON> +
            ((flag<&ast::AxisSpec::axisFunction>(ikw<"AXIS">) + ref<LPAREN> +
              assign<&ast::AxisSpec::axisName>(ref<AXIS_NUMBER>) + ref<RPAREN>) |
             assign<&ast::AxisSpec::axisName>(ref<ID> | ref<AXIS_NUMBER>)))> {};

  struct CellProperty
      : rule<create<ast::CellProperty>(assign<&ast::CellProperty::name>(
            ref<CELL_ORDINAL> | ref<VALUE> | ref<FORMATTED_VALUE> |
            identifier_text))> {};

  static constexpr auto cell_props =
      option(ref<CELL>) + ref<PROPERTIES> +
      some(append<&ast::SelectStatement::cellProps>(ref<CellProperty>),
           ref<COMMA>);

  struct SelectStatementRule
      : rule<create<ast::SelectStatement>(
            option(ref<WITH> + some(append<&ast::SelectStatement::formulas>(
                                   ref<MemberSpec> | ref<SetSpec>))) +
            ref<SELECT> +
            (flag<&ast::SelectStatement::all>(lit<"*">) | option(some(append<&ast::SelectStatement::axes>(ref<AxisSpecRule>),
                        ref<COMMA>))) +
            ref<FROM> + ((ref<LPAREN> + assign<&ast::SelectStatement::subquery>(ref<SelectStatementRule>) + ref<RPAREN>) |
                        assign<&ast::SelectStatement::cube>(ref<Name>)) +
            option(ref<WHERE> +
                   assign<&ast::SelectStatement::slicer>(ref<Expression>)) +
            option(cell_props))> {};

  struct Query : rule<ref<SelectStatementRule> + option(lit<";">)> {};
  using entry = Query;
  using ignored = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace mdx
