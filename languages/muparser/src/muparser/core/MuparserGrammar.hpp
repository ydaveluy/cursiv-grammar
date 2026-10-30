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
  struct BOOL : terminal<as<bool>(kw<"true"> | kw<"false">)> {};

  struct AND : terminal<as<ast::Operator::and_>(lit<"&&">)> {};

  struct Expression;
  struct Unary;

  struct Primary
      : rule<(lit<"("> + ref<Expression> + lit<")">) |
             create<ast::NumberLiteral>(
                 assign<&ast::NumberLiteral::value>(ref<NUMBER>)) |
             create<ast::BoolLiteral>(
                 assign<&ast::BoolLiteral::value>(ref<BOOL>)) |
             create<ast::PredefinedConstant>(
                 assign<&ast::PredefinedConstant::kind>(as<ast::Constant::e>(kw<"_e">) | as<ast::Constant::pi>(kw<"_pi">))) |
             create<ast::FunctionCall>(
                 assign<&ast::FunctionCall::name>(ref<ID>) + lit<"("> +
                 many(append<&ast::FunctionCall::args>(ref<Expression>),
                             lit<",">) +
                 lit<")">) |
             create<ast::VariableRef>(
                 assign<&ast::VariableRef::name>(ref<ID>))> {};

  struct PowExpr : rule<ref<Primary> +
                        option(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                            assign<&ast::BinaryExpr::op>(as<ast::Operator::pow>(lit<"^">)) +
                            assign<&ast::BinaryExpr::right>(ref<Unary>)))> {};

  struct Unary : rule<create<ast::UnaryExpr>(
                          assign<&ast::UnaryExpr::op>(as<ast::Operator::add>(lit<"+">) | as<ast::Operator::sub>(lit<"-">)) +
                          assign<&ast::UnaryExpr::operand>(ref<Unary>)) |
                      ref<PowExpr>> {};

  struct Binary
      : infix<Unary, ast::BinaryExpr,
              levels(left(as<ast::Operator::mul>(lit<"*">) | as<ast::Operator::div>(lit<"/">)), left(as<ast::Operator::add>(lit<"+">) | as<ast::Operator::sub>(lit<"-">)),
                     left(as<ast::Operator::lte>(lit<"<=">) | as<ast::Operator::gte>(lit<">=">) | as<ast::Operator::lt>(lit<"<">) | as<ast::Operator::gt>(lit<">">) | as<ast::Operator::eq>(lit<"==">) |
                          as<ast::Operator::neq>(lit<"!=">)),
                     left(ref<AND>), left(as<ast::Operator::or_>(lit<"||">)))> {};

  struct Conditional
      : rule<ref<Binary> +
             option(nest<ast::CondExpr, &ast::CondExpr::cond>(
                 lit<"?"> + assign<&ast::CondExpr::thenValue>(ref<Expression>) +
                 lit<":"> +
                 assign<&ast::CondExpr::elseValue>(ref<Expression>)))> {};

  struct Expression : infix<Conditional, ast::AssignExpr,
                            left(as<ast::AssignOp::assign>(lit<"=">) |
                                 as<ast::AssignOp::add>(lit<"+=">) |
                                 as<ast::AssignOp::sub>(lit<"-=">) |
                                 as<ast::AssignOp::mul>(lit<"*=">) |
                                 as<ast::AssignOp::div>(lit<"/=">)),
                            &ast::AssignExpr::target, &ast::AssignExpr::op,
                            &ast::AssignExpr::value> {};

  struct Program : rule<some(append<&ast::Program::exprs>(ref<Expression>) +
                             option(lit<",">))> {};

  using entry = Program;
  using ignored = WS;
};

}  // namespace muparser
