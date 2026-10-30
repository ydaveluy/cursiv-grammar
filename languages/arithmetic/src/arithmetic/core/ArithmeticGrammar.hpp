// The arithmetic language: its grammar.
#pragma once

#include "arithmetic/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace arithmetic {

struct Arithmetic : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  struct VARIABLE : terminal<cr<"a-zA-Z_"> + many(w)> {};

  struct SCIENTIFIC_NUMBER
      : terminal<as<double>(some(d) + option(lit<"."> + some(d)) +
                            option(cr<"eE"> + option(cr<"+-">) + some(d)))> {};

  struct LPAREN : terminal<lit<"(">> {};
  struct RPAREN : terminal<lit<")">> {};

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
                 assign<&ast::UnaryExpression::op>(as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">)) +
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
      : infix<PrimaryExpression, ast::BinaryExpression,
              levels(right(as<ast::Operator::exponent>(lit<"^">)),              //
                     left(as<ast::Operator::multiply>(lit<"*">) | as<ast::Operator::divide>(lit<"/">)),  //
                     left(as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">)))> {};

  struct Equation
      : rule<create<ast::Equation>(
            assign<&ast::Equation::left>(ref<Expression>) +
            assign<&ast::Equation::op>(as<ast::RelOp::eq>(lit<"=">) | as<ast::RelOp::gt>(lit<">">) | as<ast::RelOp::lt>(lit<"<">)) +
            assign<&ast::Equation::right>(ref<Expression>))> {};

  struct File
      : rule<create<ast::File> +
             many(append<&ast::File::equations>(ref<Equation>))> {};

  using entry = File;
  using ignored = WS;
};

} // namespace arithmetic
