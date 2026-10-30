// The tinybasic language: its grammar.
#pragma once

#include <cstdint>
#include <limits>
#include <span>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "tinybasic/core/ast.hpp"

namespace tinybasic {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

inline constexpr auto decimal = [](std::span<const std::string_view> tokens)
    -> cursiv::ConversionResult<std::int32_t> {
  std::int32_t value = 0;
  for (const auto token : tokens) for (const char digit : token) {
    if (digit < '0' || digit > '9' || value > (std::numeric_limits<std::int32_t>::max() - (digit - '0')) / 10)
      return cursiv::ConversionResult<std::int32_t>::err("The number exceeds the port's signed 32-bit range");
    value = value * 10 + (digit - '0');
  }
  return cursiv::ConversionResult<std::int32_t>::ok(value);
};

struct Tinybasic : cursiv::GrammarSpec {
  // Names are one letter, so a keyword is spelled `lit<>`, never `kw<>`: a
  // word boundary would refuse a keyword directly against a variable
  // ('GOTOA'), where maximal munch still takes the longer keyword.
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct CR : terminal<some(cr<R"(\r\n)">)> {};

  struct VAR : terminal<cr<"A-Z">> {};
  struct STRING
      : terminal<as<unquote>(lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">)> {};
  struct DIGIT : terminal<d> {};
  struct NUMBER : rule<as<decimal>(some(ref<DIGIT>))> {};

  struct RelNe : terminal<as<ast::RelOp::ne>(lit<"<>"> | lit<"><">)> {};

  // A name or a string, wherever either is legal -- an INPUT prompt/target,
  // or an assignment's two sides.
  struct Vara
      : rule<create<ast::VarRef>(assign<&ast::VarRef::name>(ref<VAR>))  //
             | create<ast::StringLit>(
                   assign<&ast::StringLit::value>(ref<STRING>))> {};

  struct Factor
      : rule<ref<Vara>  //
             | create<ast::NumberLit>(
                   assign<&ast::NumberLit::value>(ref<NUMBER>))> {};

  struct Term
      : infix<Factor, ast::BinaryExpr,
              left(as<ast::Operator::multiply>(lit<"*">) |
                   as<ast::Operator::divide>(lit<"/">))> {};

  // Only the first term of an expression may have a sign.
  struct SignedTerm
      : rule<create<ast::Negate>(
                 lit<"-"> + assign<&ast::Negate::operand>(ref<Term>))
             | (option(lit<"+">) + ref<Term>)> {};

  struct Expression
      : rule<ref<SignedTerm> + many(nest<&ast::BinaryExpr::left>(
            assign<&ast::BinaryExpr::op>(as<ast::Operator::add>(lit<"+">) |
                                         as<ast::Operator::subtract>(lit<"-">)) +
            assign<&ast::BinaryExpr::right>(ref<Term>)))> {};

  struct Statement
      : rule<create<ast::PrintStmt>(
                 lit<"PRINT"> +
                 some(append<&ast::PrintStmt::items>(ref<Expression>),
                      lit<",">))  //
             | create<ast::IfStmt>(
                   lit<"IF"> + assign<&ast::IfStmt::left>(ref<Expression>) +
                   assign<&ast::IfStmt::op>(ref<RelNe> | as<ast::RelOp::le>(lit<"<=">) |
                                            as<ast::RelOp::ge>(lit<">=">) | as<ast::RelOp::lt>(lit<"<">) |
                                            as<ast::RelOp::gt>(lit<">">) | as<ast::RelOp::eq>(lit<"=">)) +
                   assign<&ast::IfStmt::right>(ref<Expression>) +
                   option(lit<"THEN">) +
                   assign<&ast::IfStmt::then>(ref<Statement>))  //
             | create<ast::GotoStmt>(
                   lit<"GOTO"> +
                   assign<&ast::GotoStmt::target>(ref<NUMBER>))  //
             | create<ast::InputStmt>(
                   lit<"INPUT"> +
                   some(append<&ast::InputStmt::items>(ref<Vara>),
                        lit<",">))  //
             | create<ast::LetStmt>(
                   option(lit<"LET">) +
                   assign<&ast::LetStmt::target>(ref<Vara>) + lit<"="> +
                   assign<&ast::LetStmt::value>(ref<Expression>))  //
             | create<ast::GosubStmt>(
                   lit<"GOSUB"> +
                   assign<&ast::GosubStmt::target>(ref<Expression>))  //
             | (create<ast::ReturnStmt> + lit<"RETURN">)                  //
             | (create<ast::ClearStmt> + lit<"CLEAR">)                    //
             | (create<ast::ListStmt> + lit<"LIST">)                      //
             | (create<ast::RunStmt> + lit<"RUN">)                        //
             | (create<ast::EndStmt> + lit<"END">)> {};

  struct Line
      : rule<option(assign<&ast::Line::number>(ref<NUMBER>)) +
             assign<&ast::Line::statement>(ref<Statement>) + ref<CR>> {};

  struct Program
      : rule<create<ast::Program> +
             many(append<&ast::Program::lines>(ref<Line>))> {};

  using entry = Program;
  using ignored = WS;
};

}  // namespace tinybasic
