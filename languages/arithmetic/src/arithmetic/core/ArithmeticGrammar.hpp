// The arithmetic language: its grammar.
#pragma once

#include "arithmetic/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace arithmetic {

struct Arithmetic : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  struct VARIABLE : terminal<cr<"a-zA-Z_"> + many(w)> {};

  struct SCIENTIFIC_NUMBER
      : terminal<some(d) + option(lit<"."> + some(d)) +
                     option(cr<"eE"> + option(cr<"+-">) + some(d)),
                 double> {};

  struct LPAREN : terminal<lit<"(">> {};
  struct RPAREN : terminal<lit<")">> {};
  struct POW : terminal<lit<"^">, value<ast::Operator::exponent>> {};
  struct TIMES : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIV : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct PLUS : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct MINUS : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct EQ : terminal<lit<"=">, value<ast::RelOp::eq>> {};
  struct GT : terminal<lit<">">, value<ast::RelOp::gt>> {};
  struct LT : terminal<lit<"<">, value<ast::RelOp::lt>> {};

  struct Expression;
  struct UnaryExpression;
  struct GroupedExpression;

  struct AtomExpression
      : rule<create<ast::NumberLiteral>(
                 assign<&ast::NumberLiteral::value>(ref<SCIENTIFIC_NUMBER>)) //
             | create<ast::VariableExpression>(
                   assign<&ast::VariableExpression::name>(ref<VARIABLE>))> {};

  // Each sign nests one level deeper; the sign is not part of the number.
  struct UnaryExpression
      : rule<create<ast::UnaryExpression>(
                 assign<&ast::UnaryExpression::op>(ref<PLUS> | ref<MINUS>) +
                 assign<&ast::UnaryExpression::operand>(
                     ref<UnaryExpression>)) //
             | ref<AtomExpression> | ref<GroupedExpression>> {};

  struct GroupedExpression
      : rule<create<ast::GroupedExpression>(
            lit<"("> +
            assign<&ast::GroupedExpression::expression>(ref<Expression>) +
            lit<")">)> {};

  struct PrimaryExpression : rule<ref<UnaryExpression>> {};

  struct Expression
      : infix<PrimaryExpression, &ast::BinaryExpression::left,
              &ast::BinaryExpression::op, &ast::BinaryExpression::right,
              (right(ref<POW>) >>             //
               left(ref<TIMES> | ref<DIV>) >> //
               left(ref<PLUS> | ref<MINUS>))> {};

  struct EquationRule
      : rule<create<ast::Equation>(
            assign<&ast::Equation::left>(ref<Expression>) +
            assign<&ast::Equation::op>(ref<EQ> | ref<GT> | ref<LT>) +
            assign<&ast::Equation::right>(ref<Expression>))> {};

  struct FileRule
      : rule<create<ast::File> +
             many(append<&ast::File::equations>(ref<EquationRule>))> {};

  using entry = FileRule;
  using ignore = WS;
};

} // namespace arithmetic
