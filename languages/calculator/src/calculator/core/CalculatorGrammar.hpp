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

  struct COS : terminal<kw<"cos">, value<ast::Func::cos>> {};
  struct SIN : terminal<kw<"sin">, value<ast::Func::sin>> {};
  struct TAN : terminal<kw<"tan">, value<ast::Func::tan>> {};
  struct ACOS : terminal<kw<"acos">, value<ast::Func::acos>> {};
  struct ASIN : terminal<kw<"asin">, value<ast::Func::asin>> {};
  struct ATAN : terminal<kw<"atan">, value<ast::Func::atan>> {};
  struct LOG : terminal<kw<"log">, value<ast::Func::log>> {};
  struct LN : terminal<kw<"ln">, value<ast::Func::ln>> {};
  struct SQRT : terminal<kw<"sqrt">, value<ast::Func::sqrt>> {};

  struct PI : terminal<kw<"pi">, value<ast::Constant::pi>> {};
  struct EULER : terminal<kw<"e">, value<ast::Constant::euler>> {};
  struct I : terminal<kw<"i">, value<ast::Constant::i>> {};

  struct PLUS : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct MINUS : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct TIMES : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIV : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct POW : terminal<lit<"^">, value<ast::Operator::power>> {};

  struct EQ : terminal<lit<"=">, value<ast::RelOp::eq>> {};
  struct GT : terminal<lit<">">, value<ast::RelOp::gt>> {};
  struct LT : terminal<lit<"<">, value<ast::RelOp::lt>> {};

  struct Expression;
  struct SignedAtom;

  struct Expression
      : infix<SignedAtom, &ast::BinaryExpression::left,
              &ast::BinaryExpression::op, &ast::BinaryExpression::right,
              (left(ref<POW>) >>                //
               left(ref<TIMES> | ref<DIV>) >>   //
               left(ref<PLUS> | ref<MINUS>))> {};

  struct FunctionCall
      : rule<create<ast::FunctionCall>(
            assign<&ast::FunctionCall::func>(
                ref<COS> | ref<SIN> | ref<TAN> | ref<ACOS> | ref<ASIN> |
                ref<ATAN> | ref<LOG> | ref<LN> | ref<SQRT>) +
            lit<"("> +
            some(append<&ast::FunctionCall::args>(ref<Expression>),
                 lit<",">) +
            lit<")">)> {};

  // Each sign nests one level deeper; the sign is not part of the number.
  struct SignedAtom
      : rule<create<ast::UnaryExpression>(
                 assign<&ast::UnaryExpression::op>(ref<PLUS> | ref<MINUS>) +
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
                       ref<PI> | ref<EULER> | ref<I>))                     //
             | create<ast::GroupedExpression>(
                   lit<"("> +
                   assign<&ast::GroupedExpression::expression>(
                       ref<Expression>) +
                   lit<")">)> {};

  struct EquationRule
      : rule<create<ast::Equation>(
                 assign<&ast::Equation::left>(ref<Expression>) +
                 assign<&ast::Equation::op>(ref<EQ> | ref<GT> | ref<LT>) +
                 assign<&ast::Equation::right>(ref<Expression>))> {};

  using entry = EquationRule;
  using ignore = WS;
};

}  // namespace calculator
