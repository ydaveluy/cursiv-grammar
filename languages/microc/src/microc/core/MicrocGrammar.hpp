// The microc language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "microc/core/ast.hpp"

namespace microc {

struct Microc : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct ID : terminal<some(cr<"a-z">)> {};
  struct INT : terminal<some(d)> {};


  struct Statement;
  struct Expr;

  struct Ident : rule<assign<&ast::Ident::name>(ref<ID>)> {};
  struct IntegerLiteral : rule<assign<&ast::IntLit::value>(ref<INT>)> {};

  // Parens are grouping only: the alternative adopts the inner expression's
  // own node, same as `if`/`while` below.
  struct Term : rule<(lit<"("> + ref<Expr> + lit<")">)  //
                     | ref<Ident>                       //
                     | ref<IntegerLiteral>> {};

  struct Sum : infix<Term, ast::BinaryExpr,
                     left(as<ast::BinOp::add>(lit<"+">) |
                          as<ast::BinOp::subtract>(lit<"-">))> {};

  // At most one comparison: `a<b<c` is not in the language.
  struct Test
      : rule<create<ast::BinaryExpr>(assign<&ast::BinaryExpr::left>(ref<Sum>) +
                                     assign<&ast::BinaryExpr::op>(as<ast::BinOp::less>(lit<"<">)) +
                                     assign<&ast::BinaryExpr::right>(ref<Sum>))  //
             | ref<Sum>> {};

  struct Expr
      : rule<create<ast::Assign>(
                 assign<&ast::Assign::target>(ref<Ident>) + lit<"="> +
                 assign<&ast::Assign::value>(ref<Expr>))  //
             | ref<Test>> {};

  struct ExpressionStatement
      : rule<assign<&ast::ExprStmt::value>(ref<Expr>) + lit<";">> {};

  struct BlockStatement
      : rule<lit<"{"> + many(append<&ast::Block::statements>(ref<Statement>)) +
             lit<"}">> {};

  struct IfStatement
      : rule<kw<"if"> + lit<"("> + assign<&ast::If::cond>(ref<Expr>) +
             lit<")"> + assign<&ast::If::thenBranch>(ref<Statement>) +
             option(commit(kw<"else">, assign<&ast::If::elseBranch>(ref<Statement>)))> {};

  struct WhileStatement
      : rule<kw<"while"> + lit<"("> + assign<&ast::While::cond>(ref<Expr>) +
             lit<")"> + assign<&ast::While::body>(ref<Statement>)> {};

  struct Statement : rule<ref<IfStatement>      //
                          | ref<WhileStatement>  //
                          | ref<BlockStatement>  //
                          | ref<ExpressionStatement>> {};

  struct Program
      : rule<some(append<&ast::Program::statements>(ref<Statement>))> {};

  using entry = Program;
  using ignored = WS;
};

}  // namespace microc
