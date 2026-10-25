#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "muparser/core/ast.hpp"

namespace muparser {

struct Muparser : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<"\u0001-\u000D ">)> {};
  struct ID : terminal<cr<"a-zA-Z_"> + many(cr<"a-zA-Z_0-9">)> {};
  struct NUMBER : terminal<((some(d) + option(lit<"."> + many(d))) |
                            (lit<"."> + some(d))) +
                           option(cr<"eE"> + option(cr<"+-">) + some(d))> {};
  struct BOOL : terminal<kw<"true"> | kw<"false">, bool> {};
  struct E : terminal<kw<"_e">, value<ast::Constant::e>> {};
  struct PI : terminal<kw<"_pi">, value<ast::Constant::pi>> {};

  struct POW : terminal<lit<"^">, value<ast::Operator::pow>> {};
  struct MUL : terminal<lit<"*">, value<ast::Operator::mul>> {};
  struct DIV : terminal<lit<"/">, value<ast::Operator::div>> {};
  struct ADD : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct SUB : terminal<lit<"-">, value<ast::Operator::sub>> {};
  struct LTEQ : terminal<lit<"<=">, value<ast::Operator::lte>> {};
  struct GTEQ : terminal<lit<">=">, value<ast::Operator::gte>> {};
  struct LT : terminal<lit<"<">, value<ast::Operator::lt>> {};
  struct GT : terminal<lit<">">, value<ast::Operator::gt>> {};
  struct EQ : terminal<lit<"==">, value<ast::Operator::eq>> {};
  struct NEQ : terminal<lit<"!=">, value<ast::Operator::neq>> {};
  struct AND : terminal<lit<"&&">, value<ast::Operator::and_>> {};
  struct OR : terminal<lit<"||">, value<ast::Operator::or_>> {};

  struct ASSIGN : terminal<lit<"=">, value<ast::AssignOp::assign>> {};
  struct ASSIGNADD : terminal<lit<"+=">, value<ast::AssignOp::add>> {};
  struct ASSIGNSUB : terminal<lit<"-=">, value<ast::AssignOp::sub>> {};
  struct ASSIGNMUL : terminal<lit<"*=">, value<ast::AssignOp::mul>> {};
  struct ASSIGNDIV : terminal<lit<"/=">, value<ast::AssignOp::div>> {};

  struct Expression;
  struct Unary;

  struct Primary
      : rule<(lit<"("> + ref<Expression> + lit<")">) |
             create<ast::NumberLiteral>(
                 assign<&ast::NumberLiteral::value>(ref<NUMBER>)) |
             create<ast::BoolLiteral>(
                 assign<&ast::BoolLiteral::value>(ref<BOOL>)) |
             create<ast::PredefinedConstant>(
                 assign<&ast::PredefinedConstant::kind>(ref<E> | ref<PI>)) |
             create<ast::FunctionCall>(
                 assign<&ast::FunctionCall::name>(ref<ID>) + lit<"("> +
                 option(some(append<&ast::FunctionCall::args>(ref<Expression>),
                             lit<",">)) +
                 lit<")">) |
             create<ast::VariableRef>(
                 assign<&ast::VariableRef::name>(ref<ID>))> {};

  struct PowExpr : rule<ref<Primary> +
                        option(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                            assign<&ast::BinaryExpr::op>(ref<POW>) +
                            assign<&ast::BinaryExpr::right>(ref<Unary>)))> {};

  struct Unary : rule<create<ast::UnaryExpr>(
                          assign<&ast::UnaryExpr::op>(ref<ADD> | ref<SUB>) +
                          assign<&ast::UnaryExpr::operand>(ref<Unary>)) |
                      ref<PowExpr>> {};

  struct Binary
      : infix<Unary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<MUL> | ref<DIV>) >> left(ref<ADD> | ref<SUB>) >>
               left(ref<LTEQ> | ref<GTEQ> | ref<LT> | ref<GT> | ref<EQ> |
                    ref<NEQ>) >>
               left(ref<AND>) >> left(ref<OR>))> {};

  struct Conditional
      : rule<ref<Binary> +
             option(nest<ast::CondExpr, &ast::CondExpr::cond>(
                 lit<"?"> + assign<&ast::CondExpr::thenValue>(ref<Expression>) +
                 lit<":"> +
                 assign<&ast::CondExpr::elseValue>(ref<Expression>)))> {};

  struct Expression : infix<Conditional, &ast::AssignExpr::target,
                            &ast::AssignExpr::op, &ast::AssignExpr::value,
                            left(ref<ASSIGN> | ref<ASSIGNADD> | ref<ASSIGNSUB> |
                                 ref<ASSIGNMUL> | ref<ASSIGNDIV>)> {};

  struct Program : rule<some(append<&ast::Program::exprs>(ref<Expression>) +
                             option(lit<",">))> {};

  using entry = Program;
  using ignore = WS;
};

}  // namespace muparser
