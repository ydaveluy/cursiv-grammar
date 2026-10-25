// The tiny language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "tiny/core/ast.hpp"

namespace tiny {

struct Tiny : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n)">)> {};

  struct ID : terminal<some(cr<"a-zA-Z">)> {};
  struct INTEGER : terminal<some(d)> {};
  struct IntegerRule : rule<option(lit<"-">) + ref<INTEGER>, std::string_view> {};

  struct ADD : terminal<lit<"+">, value<ast::Op::add>> {};
  struct SUBTRACT : terminal<lit<"-">, value<ast::Op::subtract>> {};

  struct Factor
      : rule<create<ast::VarRef>(assign<&ast::VarRef::name>(ref<ID>))  //
             | create<ast::IntegerLiteral>(
                   assign<&ast::IntegerLiteral::value>(ref<IntegerRule>))> {};

  struct ExprRule
      : infix<Factor, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<ADD> | ref<SUBTRACT>)> {};

  struct AssignStmt
      : rule<assign<&ast::AssignStmt::target>(ref<ID>) + lit<":="> +
             assign<&ast::AssignStmt::value>(ref<ExprRule>)> {};

  struct ReadStmt
      : rule<kw<"READ"> +
             some(append<&ast::ReadStmt::names>(ref<ID>), lit<",">)> {};

  struct WriteStmt
      : rule<kw<"WRITE"> +
             some(append<&ast::WriteStmt::values>(ref<ExprRule>), lit<",">)> {
  };

  struct StmtRule : rule<ref<AssignStmt> | ref<ReadStmt> | ref<WriteStmt>> {};

  struct ProgramRule
      : rule<create<ast::Program> + kw<"BEGIN"> +
             some(append<&ast::Program::statements>(ref<StmtRule>)) +
             kw<"END">> {};

  using entry = ProgramRule;
  using ignore = WS;
};

}  // namespace tiny
