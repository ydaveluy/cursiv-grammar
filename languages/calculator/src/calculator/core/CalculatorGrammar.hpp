// The calculator language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "calculator/core/ast.hpp"

namespace calculator {

struct Calculator : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct VARIABLE : terminal<cr<"a-zA-Z_"> + many(w)> {};

  // The exponent's run is a full NUMBER too, decimal point included: a
  // scientific literal like "12.3e13.2" lexes as one token, unevaluated.
  static constexpr auto NumberPart = some(d) + option(lit<"."> + some(d));
  struct SCIENTIFIC_NUMBER
      : terminal<NumberPart +
                 option(cr<"eE"> + option(cr<"+-">) + NumberPart)> {};





  struct Expression;
  struct SignedAtom;

  struct Expression
      : infix<SignedAtom, ast::BinaryExpression,
              levels(left(as<ast::Operator::power>(lit<"^">)),               //
                     left(as<ast::Operator::multiply>(lit<"*">) | as<ast::Operator::divide>(lit<"/">)),  //
                     left(as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">)))> {};

  struct FunctionCall
      : rule<create<ast::FunctionCall>(
            assign<&ast::FunctionCall::func>(
                as<ast::Func::cos>(kw<"cos">) | as<ast::Func::sin>(kw<"sin">) | as<ast::Func::tan>(kw<"tan">) | as<ast::Func::acos>(kw<"acos">) | as<ast::Func::asin>(kw<"asin">) |
                as<ast::Func::atan>(kw<"atan">) | as<ast::Func::log>(kw<"log">) | as<ast::Func::ln>(kw<"ln">) | as<ast::Func::sqrt>(kw<"sqrt">)) +
            lit<"("> +
            some(append<&ast::FunctionCall::args>(ref<Expression>),
                 lit<",">) +
            lit<")">)> {};

  // Each sign nests one level deeper; the sign is not part of the number.
  struct SignedAtom
      : rule<create<ast::UnaryExpression>(
                 assign<&ast::UnaryExpression::op>(as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">)) +
                 assign<&ast::UnaryExpression::operand>(
                     ref<SignedAtom>))                                     //
             | ref<FunctionCall>                                          //
             | create<ast::NumberLiteral>(
                   assign<&ast::NumberLiteral::value>(
                       ref<SCIENTIFIC_NUMBER>))                            //
             | create<ast::VariableExpression>(
                   assign<&ast::VariableExpression::name>(ref<VARIABLE>))  //
             | create<ast::ConstantExpression>(
                   assign<&ast::ConstantExpression::value>(
                       as<ast::Constant::pi>(kw<"pi">) | as<ast::Constant::euler>(kw<"e">) | as<ast::Constant::i>(kw<"i">)))                     //
             | create<ast::GroupedExpression>(
                   lit<"("> +
                   assign<&ast::GroupedExpression::expression>(
                       ref<Expression>) +
                   lit<")">)> {};

  struct Equation
      : rule<create<ast::Equation>(
                 assign<&ast::Equation::left>(ref<Expression>) +
                 assign<&ast::Equation::op>(as<ast::RelOp::eq>(lit<"=">) | as<ast::RelOp::gt>(lit<">">) | as<ast::RelOp::lt>(lit<"<">)) +
                 assign<&ast::Equation::right>(ref<Expression>))> {};

  using entry = Equation;
  using ignored = WS;
};

}  // namespace calculator
