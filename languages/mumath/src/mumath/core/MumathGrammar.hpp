// The mumath language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "mumath/core/ast.hpp"

namespace mumath {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

struct Mumath : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  // '%' opens and the first '%' after it closes; comments do not nest.
  struct COMMENT : terminal<through(lit<"%">, lit<"%">)> {};

  // The trailing chars go beyond the default word class, so a keyword must
  // not be taken as their prefix.
  static constexpr auto word_boundary = cr<"0-9A-Za-z_#}">;
  static constexpr auto Reserved =
      kw<"FUNCTION"> | kw<"ENDFUN"> | kw<"BLOCK"> | kw<"ENDBLOCK"> |
      kw<"LOOP"> | kw<"ENDLOOP"> | kw<"WHEN"> | kw<"EXIT"> |
      kw<"AND"> | kw<"OR"> | kw<"NOT">;
  struct RAW_ID : terminal<cr<"A-Za-z@{#"> + many(cr<"A-Za-z0-9#}">) +
                           option(lit<"["> + some(d) + lit<"]">)> {};
  struct ID : terminal<!Reserved + ref<RAW_ID>> {};

  struct NUMBER : terminal<as<double>(some(d))> {};

  struct STRING
      : terminal<as<unquote>(lit<"\""> + many(cr<R"(^")">) + lit<"\"">)> {};

  // Default muSIMP-79 binding powers, with the imported mod/comparison extensions.
  static constexpr auto RelOp = kw<"EQ"> | lit<"=="> | lit<"<>"> | lit<"<="> |
                                lit<"<"> | lit<">="> | lit<">"> | lit<"=">;
  static constexpr auto AddOp = lit<"+"> | lit<"-">;
  static constexpr auto MulOp = lit<"*"> | lit<"/"> | kw<"mod">;

  struct Expression;
  struct Conjunction;
  struct Not;
  struct Comparison;
  struct SimpleExpression;
  struct Term;
  struct Power;
  struct Unary;
  struct Factor;
  struct FunctionDesignator;
  struct Assignment;
  struct ExpressionStatement;
  struct CallStatement;
  struct Stmt;
  struct Loop;
  struct When;
  struct Block;
  struct FunctionDefinition;
  struct ProgramItem;

  struct Factor
      : rule<create<ast::NumberLiteral>(
                 assign<&ast::NumberLiteral::value>(ref<NUMBER>))  //
             | create<ast::StringLiteral>(
                   assign<&ast::StringLiteral::text>(ref<STRING>))  //
             | create<ast::QuotedName>(
                   lit<"'"> + assign<&ast::QuotedName::name>(ref<RAW_ID>))  //
             | create<ast::QuotedString>(
                   lit<"'"> + assign<&ast::QuotedString::text>(ref<STRING>))  //
             | create<ast::GroupedExpression>(
                   lit<"("> +
                   assign<&ast::GroupedExpression::expression>(ref<Expression>) +
                   lit<")">)  //
             | ref<FunctionDesignator>                                //
             | create<ast::NameExpression>(
                   assign<&ast::NameExpression::name>(ref<ID>))> {};

  // muSIMP-79 table: power 140/139, prefix +/- 130, product 120.
  // A signed power's right operand may itself start with a prefix sign.
  struct Power
      : rule<ref<Factor> +
             option(nest<ast::BinaryExpression, &ast::BinaryExpression::left>(
                 assign<&ast::BinaryExpression::op>(lit<"^">) +
                 assign<&ast::BinaryExpression::right>(ref<Unary>)))> {};

  struct Unary
      : rule<create<ast::NegateExpression>(
                 lit<"-"> +
                 assign<&ast::NegateExpression::operand>(ref<Unary>)) |
             create<ast::PositiveExpression>(
                 lit<"+"> +
                 assign<&ast::PositiveExpression::operand>(ref<Unary>)) |
             ref<Power>> {};

  struct Term : infix<Unary, ast::BinaryExpression, left(MulOp)> {};

  struct SimpleExpression
      : infix<Term, ast::BinaryExpression, left(AddOp)> {};

  struct Comparison
      : rule<ref<SimpleExpression> +
             many(nest<ast::BinaryExpression, &ast::BinaryExpression::left>(
                 assign<&ast::BinaryExpression::op>(RelOp) +
                 assign<&ast::BinaryExpression::right>(ref<SimpleExpression>)))> {
  };

  // NOT is looser than comparison, so `NOT A=B` reads `NOT(A=B)`, not
  // `(NOT A)=B`; stacks (`NOT NOT A`).
  struct Not
      : rule<create<ast::NotExpression>(
                 kw<"NOT"> +
                 assign<&ast::NotExpression::operand>(ref<Not>))  //
             | ref<Comparison>> {};

  struct Conjunction : infix<Not, ast::BinaryExpression,
                             left(kw<"AND">)> {};

  // AND/OR are the loosest operators: `A<=B AND A>=B` reads
  // `(A<=B) AND (A>=B)`.
  struct Expression
      : infix<Conjunction, ast::BinaryExpression, left(kw<"OR">)> {};

  struct FunctionDesignator
      : rule<assign<&ast::CallExpression::func>(ref<ID>) + lit<"("> +
             many(append<&ast::CallExpression::args>(ref<Expression> |
                                                      ref<Assignment>),
                  lit<",">) +
             lit<")">> {};

  struct Assignment
      : rule<some(append<&ast::Assignment::targets>(ref<ID>) + lit<":">) +
             assign<&ast::Assignment::expr>(ref<Expression>)> {};

  struct ExpressionStatement
      : rule<assign<&ast::ExpressionStatement::expr>(ref<Expression>)> {};

  // Compatibility entry for clients; program items now share Stmt.
  struct CallStatement
      : rule<assign<&ast::ExpressionStatement::expr>(ref<FunctionDesignator>)> {
  };

  // functionDesignator is dropped from the choice: it is already reachable
  // through expression, and keeping both spells one construct twice.
  struct Stmt : rule<ref<Loop> | ref<When> | ref<Block> | ref<Assignment> |
                     ref<ExpressionStatement>> {};

  struct Block
      : rule<kw<"BLOCK"> +
             many(append<&ast::Block::body>(ref<Stmt>), lit<",">) +
             option(lit<",">) + kw<"ENDBLOCK">> {};

  struct Loop
      : rule<kw<"LOOP"> +
             many(append<&ast::Loop::body>(ref<Stmt>), lit<",">) +
             option(lit<",">) + kw<"ENDLOOP">> {};

  // MATCH(EXIT) closes one list: the first expression is the condition,
  // and any remaining expressions form the conditional exit's body.
  struct When
      : rule<kw<"WHEN"> +
             assign<&ast::When::condition>(ref<Expression>) +
             option(lit<","> +
                    some(append<&ast::When::body>(ref<Stmt>), lit<",">)) +
             option(lit<",">) + kw<"EXIT">> {};

  struct FunctionDefinition
      : rule<kw<"FUNCTION"> +
             assign<&ast::FunctionDefinition::name>(ref<ID>) + lit<"("> +
             many(append<&ast::FunctionDefinition::params>(ref<ID>), lit<",">) +
             lit<")"> + lit<","> +
             many(append<&ast::FunctionDefinition::body>(ref<Stmt>), lit<",">) +
             option(lit<",">) + kw<"ENDFUN">> {};

  struct ProgramItem : rule<ref<FunctionDefinition> | ref<Stmt>> {};

  struct Program
      : rule<many(append<&ast::Program::statements>(ref<ProgramItem>) +
                  (lit<";"> | lit<"$"> | lit<"&">))> {};

  using entry = Program;
  using ignored = list<WS, COMMENT>;
};

}  // namespace mumath
