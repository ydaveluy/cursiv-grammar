// The tinyc language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "tinyc/core/ast.hpp"

namespace tinyc {

struct Tinyc : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct ID : terminal<some(cr<"a-z">)> {};
  struct INT : terminal<some(cr<"0-9">)> {};

  struct Statement;
  struct Expr;

  struct Identifier : rule<assign<&ast::Ident::name>(ref<ID>)> {};
  struct IntegerLiteral : rule<assign<&ast::IntLit::value>(ref<INT>)> {};

  struct ParenExpr : rule<lit<"("> + ref<Expr> + lit<")">> {};

  struct Term : rule<ref<Identifier> | ref<IntegerLiteral> | ref<ParenExpr>> {};

  static constexpr auto OpAdd = lit<"+"> | lit<"-">;
  struct Sum : infix<Term, ast::BinaryExpr, left(OpAdd)> {};

  struct Test : infix<Sum, ast::BinaryExpr, nonassoc(lit<"<">)> {};

  struct Expr : rule<create<ast::Assign>(
                         assign<&ast::Assign::target>(ref<Identifier>) +
                         lit<"="> + assign<&ast::Assign::value>(ref<Expr>)) //
                     | ref<Test>> {};

  struct ExprStatement
      : rule<option(assign<&ast::ExprStmt::value>(ref<Expr>)) + lit<";">> {};

  struct BlockStatement
      : rule<lit<"{"> +
             many(append<&ast::BlockStmt::statements>(ref<Statement>)) +
             lit<"}">> {};

  struct IfStatement
      : rule<kw<"if"> + assign<&ast::IfStmt::cond>(ref<ParenExpr>) +
             assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
             option(kw<"else"> +
                    assign<&ast::IfStmt::elseBranch>(ref<Statement>))> {};

  struct WhileStatement
      : rule<kw<"while"> + assign<&ast::WhileStmt::cond>(ref<ParenExpr>) +
             assign<&ast::WhileStmt::body>(ref<Statement>)> {};

  struct DoStatement
      : rule<kw<"do"> + assign<&ast::DoStmt::body>(ref<Statement>) +
             kw<"while"> + assign<&ast::DoStmt::cond>(ref<ParenExpr>) +
             lit<";">> {};

  struct Statement : rule<ref<IfStatement>      //
                          | ref<WhileStatement> //
                          | ref<DoStatement>    //
                          | ref<BlockStatement> //
                          | ref<ExprStatement>> {};

  struct Program
      : rule<create<ast::Program> +
             many(append<&ast::Program::statements>(ref<Statement>))> {};

  using entry = Program;
  using ignored = WS;
};

} // namespace tinyc
