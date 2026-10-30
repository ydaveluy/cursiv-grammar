// The tiny language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "tiny/core/ast.hpp"

namespace tiny {

struct Tiny : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n)">)> {};

  struct ID : terminal<some(cr<"a-zA-Z">)> {};
  struct INTEGER : terminal<some(d)> {};
  struct Integer : rule<as<std::string_view>(option(lit<"-">) + ref<INTEGER>)> {
  };

  struct Factor
      : rule<create<ast::VarRef>(assign<&ast::VarRef::name>(ref<ID>))  //
             | create<ast::IntegerLiteral>(
                   assign<&ast::IntegerLiteral::value>(ref<Integer>))> {};

  struct Expr : infix<Factor, ast::BinaryExpr,
                      left(as<ast::Op::add>(lit<"+">) |
                           as<ast::Op::subtract>(lit<"-">))> {};

  struct AssignStmt
      : rule<assign<&ast::AssignStmt::target>(ref<ID>) + lit<":="> +
             assign<&ast::AssignStmt::value>(ref<Expr>)> {};

  struct ReadStmt
      : rule<kw<"READ"> +
             some(append<&ast::ReadStmt::names>(ref<ID>), lit<",">)> {};

  struct WriteStmt
      : rule<kw<"WRITE"> +
             some(append<&ast::WriteStmt::values>(ref<Expr>), lit<",">)> {
  };

  struct Stmt : rule<ref<AssignStmt> | ref<ReadStmt> | ref<WriteStmt>> {};

  struct Program
      : rule<create<ast::Program> + kw<"BEGIN"> +
             some(append<&ast::Program::statements>(ref<Stmt>)) +
             kw<"END">> {};

  using entry = Program;
  using ignored = WS;
};

}  // namespace tiny
