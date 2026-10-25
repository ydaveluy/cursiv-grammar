// The tinybasic language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "tinybasic/core/ast.hpp"

namespace tinybasic {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Tinybasic : cursiv::GrammarSpec {
  // Names are one letter, so a keyword is spelled `lit<>`, never `kw<>`: a
  // word boundary would refuse a keyword directly against a variable
  // ('GOTOA'), where maximal munch still takes the longer keyword.
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct CR : terminal<some(cr<R"(\r\n)">)> {};

  struct VAR : terminal<cr<"A-Z">> {};
  struct STRING : terminal<lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">, Unquoted> {};
  struct NUMBER : terminal<some(d), std::int32_t> {};

  struct MUL : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIV : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct ADD : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct SUB : terminal<lit<"-">, value<ast::Operator::subtract>> {};

  struct RelNe : terminal<lit<"<>"> | lit<"><">, value<ast::RelOp::ne>> {};
  struct RelLe : terminal<lit<"<=">, value<ast::RelOp::le>> {};
  struct RelGe : terminal<lit<">=">, value<ast::RelOp::ge>> {};
  struct RelLt : terminal<lit<"<">, value<ast::RelOp::lt>> {};
  struct RelGt : terminal<lit<">">, value<ast::RelOp::gt>> {};
  struct RelEq : terminal<lit<"=">, value<ast::RelOp::eq>> {};

  // A name or a string, wherever either is legal -- an INPUT prompt/target,
  // or an assignment's two sides.
  struct VaraRule
      : rule<create<ast::VarRef>(assign<&ast::VarRef::name>(ref<VAR>))  //
             | create<ast::StringLit>(
                   assign<&ast::StringLit::value>(ref<STRING>))> {};

  struct Factor
      : rule<ref<VaraRule>  //
             | create<ast::NumberLit>(
                   assign<&ast::NumberLit::value>(ref<NUMBER>))> {};

  // TinyBASIC restricts a leading sign to an expression's first term; a
  // sign here may lead any factor instead, which accepts a few more strings
  // ('2*-3') for the same values ('-(a*b)' is '(-a)*b').
  struct SignedFactor
      : rule<create<ast::Negate>(
                 lit<"-"> + assign<&ast::Negate::operand>(ref<Factor>))  //
             | (option(lit<"+">) + ref<Factor>)> {};

  struct ExpressionRule
      : infix<SignedFactor, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<MUL> | ref<DIV>) >> left(ref<ADD> | ref<SUB>))> {};

  struct StatementRule
      : rule<create<ast::PrintStmt>(
                 lit<"PRINT"> +
                 some(append<&ast::PrintStmt::items>(ref<ExpressionRule>),
                      lit<",">))  //
             | create<ast::IfStmt>(
                   lit<"IF"> + assign<&ast::IfStmt::left>(ref<ExpressionRule>) +
                   assign<&ast::IfStmt::op>(ref<RelNe> | ref<RelLe> |
                                            ref<RelGe> | ref<RelLt> |
                                            ref<RelGt> | ref<RelEq>) +
                   assign<&ast::IfStmt::right>(ref<ExpressionRule>) +
                   option(lit<"THEN">) +
                   assign<&ast::IfStmt::then>(ref<StatementRule>))  //
             | create<ast::GotoStmt>(
                   lit<"GOTO"> +
                   assign<&ast::GotoStmt::target>(ref<NUMBER>))  //
             | create<ast::InputStmt>(
                   lit<"INPUT"> +
                   some(append<&ast::InputStmt::items>(ref<VaraRule>),
                        lit<",">))  //
             | create<ast::LetStmt>(
                   option(lit<"LET">) +
                   assign<&ast::LetStmt::target>(ref<VaraRule>) + lit<"="> +
                   assign<&ast::LetStmt::value>(ref<ExpressionRule>))  //
             | create<ast::GosubStmt>(
                   lit<"GOSUB"> +
                   assign<&ast::GosubStmt::target>(ref<ExpressionRule>))  //
             | (create<ast::ReturnStmt> + lit<"RETURN">)                  //
             | (create<ast::ClearStmt> + lit<"CLEAR">)                    //
             | (create<ast::ListStmt> + lit<"LIST">)                      //
             | (create<ast::RunStmt> + lit<"RUN">)                        //
             | (create<ast::EndStmt> + lit<"END">)> {};

  struct LineRule
      : rule<option(assign<&ast::Line::number>(ref<NUMBER>)) +
             assign<&ast::Line::statement>(ref<StatementRule>) + ref<CR>> {};

  struct ProgramRule
      : rule<create<ast::Program> +
             many(append<&ast::Program::lines>(ref<LineRule>))> {};

  using entry = ProgramRule;
  using ignore = WS;
};

}  // namespace tinybasic
