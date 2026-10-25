// The mumath language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "mumath/core/ast.hpp"

namespace mumath {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Mumath : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  // '%' opens and the first '%' after it closes; comments do not nest.
  struct COMMENT : terminal<through(lit<"%">, lit<"%">)> {};

  // The trailing chars go beyond the default word class, so a keyword must
  // not be taken as their prefix.
  static constexpr auto word_boundary = cr<"0-9A-Za-z_#}">;
  struct ID : terminal<cr<"A-Z@{#"> + many(cr<"A-Z0-9#}">) +
                       option(lit<"["> + some(d) + lit<"]">)> {};

  struct NUMBER : terminal<some(d), double> {};

  struct STRING : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">, Unquoted> {
  };

  // Binding powers, loosest to tightest: OR < AND < NOT <
  // comparison < binary +/- < */,mod < unary +/- < ^ (right-assoc).
  static constexpr auto RelOp = kw<"EQ"> | lit<"=="> | lit<"<>"> | lit<"<="> |
                                lit<"<"> | lit<">="> | lit<">"> | lit<"=">;
  static constexpr auto AddOp = lit<"+"> | lit<"-">;
  static constexpr auto MulOp = lit<"*"> | lit<"/"> | kw<"mod">;

  struct Expression;
  struct Conjunction;
  struct NotRule;
  struct Comparison;
  struct SimpleExpression;
  struct Term;
  struct Power;
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
                   lit<"'"> + assign<&ast::QuotedName::name>(ref<ID>))  //
             | create<ast::QuotedString>(
                   lit<"'"> + assign<&ast::QuotedString::text>(ref<STRING>))  //
             | create<ast::GroupedExpression>(
                   lit<"("> +
                   assign<&ast::GroupedExpression::expression>(ref<Expression>) +
                   lit<")">)  //
             | ref<FunctionDesignator>                                //
             | create<ast::NameExpression>(
                   assign<&ast::NameExpression::name>(ref<ID>))> {};

  // `^` binds tighter than `*`/`/` and is right-associative: `2^3^2` reads
  // `2^(3^2)`.
  struct Power : infix<Factor, &ast::BinaryExpression::left,
                       &ast::BinaryExpression::op, &ast::BinaryExpression::right,
                       right(lit<"^">)> {};

  struct Term : infix<Power, &ast::BinaryExpression::left,
                      &ast::BinaryExpression::op, &ast::BinaryExpression::right,
                      left(MulOp)> {};

  // A leading sign applies to the whole term, `^` included: `-2^2` reads
  // `-(2^2)`. Only the first term of a chain may carry one (Pascal's own
  // `simple-expression = [sign] term {addop term}` shape).
  struct SimpleExpression
      : rule<(create<ast::NegateExpression>(
                  lit<"-"> +
                  assign<&ast::NegateExpression::operand>(ref<Term>))  //
              | ref<Term>) +
             many(nest<ast::BinaryExpression, &ast::BinaryExpression::left>(
                 assign<&ast::BinaryExpression::op>(AddOp) +
                 assign<&ast::BinaryExpression::right>(ref<Term>)))> {};

  struct Comparison
      : rule<ref<SimpleExpression> +
             many(nest<ast::BinaryExpression, &ast::BinaryExpression::left>(
                 assign<&ast::BinaryExpression::op>(RelOp) +
                 assign<&ast::BinaryExpression::right>(ref<SimpleExpression>)))> {
  };

  // NOT is looser than comparison, so `NOT A=B` reads `NOT(A=B)`, not
  // `(NOT A)=B`; stacks (`NOT NOT A`).
  struct NotRule
      : rule<create<ast::NotExpression>(
                 kw<"NOT"> +
                 assign<&ast::NotExpression::operand>(ref<NotRule>))  //
             | ref<Comparison>> {};

  struct Conjunction : infix<NotRule, &ast::BinaryExpression::left,
                             &ast::BinaryExpression::op,
                             &ast::BinaryExpression::right,
                             left(kw<"AND">)> {};

  // AND/OR are the loosest operators: `A<=B AND A>=B` reads
  // `(A<=B) AND (A>=B)`.
  struct Expression : infix<Conjunction, &ast::BinaryExpression::left,
                            &ast::BinaryExpression::op,
                            &ast::BinaryExpression::right, left(kw<"OR">)> {};

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

  // A functionDesignator used as a statement, at program level -- the only
  // expression form accepted there.
  struct CallStatement
      : rule<assign<&ast::ExpressionStatement::expr>(ref<FunctionDesignator>)> {
  };

  // functionDesignator is dropped from the choice: it is already reachable
  // through expression, and keeping both spells one construct twice.
  struct Stmt : rule<ref<Loop> | ref<When> | ref<Block> | ref<Assignment> |
                     ref<ExpressionStatement>> {};

  // The statement list never ends on a trailing ',', so its own separators
  // plus the one before ENDBLOCK are exactly one ',' after every item.
  struct Block : rule<kw<"BLOCK"> +
                      some(append<&ast::Block::body>(ref<Stmt>) + lit<",">) +
                      kw<"ENDBLOCK">> {};

  struct Loop
      : rule<kw<"LOOP"> +
             some(append<&ast::Loop::body>(ref<Stmt>), lit<",">) +
             option(lit<",">) + kw<"ENDLOOP">> {};

  struct When : rule<kw<"WHEN"> +
                     assign<&ast::When::condition>(ref<Expression>) +
                     option(lit<",">) + kw<"EXIT"> + lit<","> +
                     some(append<&ast::When::body>(ref<Stmt>), lit<",">) +
                     option(lit<",">) + kw<"EXIT">> {};

  struct FunctionDefinition
      : rule<kw<"FUNCTION"> +
             assign<&ast::FunctionDefinition::name>(ref<ID>) + lit<"("> +
             many(append<&ast::FunctionDefinition::params>(ref<ID>), lit<",">) +
             lit<")"> + lit<","> +
             some(append<&ast::FunctionDefinition::body>(ref<Stmt>), lit<",">) +
             option(lit<",">) + kw<"ENDFUN">> {};

  struct ProgramItem
      : rule<ref<FunctionDefinition> | ref<Assignment> | ref<CallStatement>> {};

  struct ProgramRule
      : rule<many(append<&ast::Program::statements>(ref<ProgramItem>) +
                  (lit<";"> | lit<"$">))> {};

  using entry = ProgramRule;
  using ignore = list<WS, COMMENT>;
};

}  // namespace mumath
