// The pl0 language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pl0/core/ast.hpp"

namespace pl0 {

struct Pl0 : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  static constexpr auto reserved = ikw<"WRITE"> | ikw<"WHILE"> | ikw<"DO"> |
      ikw<"IF"> | ikw<"THEN"> | ikw<"ODD"> | ikw<"BEGIN"> | ikw<"END"> |
      ikw<"CALL"> | ikw<"CONST"> | ikw<"VAR"> | ikw<"PROCEDURE">;
  struct ID : terminal<!reserved + some(cr<"a-zA-Z">)> {};
  struct NUMBER : terminal<some(d)> {};

  struct Expression;

  struct Factor : rule<create<ast::NameExpr>(
                           assign<&ast::NameExpr::name>(ref<ID>))  //
                       | create<ast::NumberExpr>(
                             assign<&ast::NumberExpr::value>(ref<NUMBER>))  //
                       | create<ast::GroupedExpr>(
                             lit<"("> +
                             assign<&ast::GroupedExpr::expression>(
                                 ref<Expression>) +
                             lit<")">)> {};

  struct Term : infix<Factor, ast::BinaryExpr,
                      left(as<ast::Operator::multiply>(lit<"*">) | as<ast::Operator::divide>(lit<"/">))> {};

  // A left-recursive chain, kept apart from `Term`'s `infix<>` table because
  // the sign binds to the first term only: `-1+2` is `(-1)+2`, not
  // `-(1+2)`.
  struct Expression
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">)) +
                 assign<&ast::UnaryExpr::operand>(ref<Term>))  //
             | ref<Term>                                       //
             | create<ast::BinaryExpr>(
                   assign<&ast::BinaryExpr::left>(ref<Expression>) +
                   assign<&ast::BinaryExpr::op>(as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">)) +
                   assign<&ast::BinaryExpr::right>(ref<Term>))> {};

  struct Condition
      : rule<create<ast::OddCond>(
                 ikw<"ODD"> +
                 assign<&ast::OddCond::operand>(ref<Expression>))  //
             | create<ast::CompareCond>(
                   assign<&ast::CompareCond::left>(ref<Expression>) +
                   assign<&ast::CompareCond::op>(
                       as<ast::RelOp::le>(lit<"<=">) | as<ast::RelOp::ge>(lit<">=">) | as<ast::RelOp::ne>(lit<"#">) |
                       as<ast::RelOp::eq>(lit<"=">) | as<ast::RelOp::lt>(lit<"<">) | as<ast::RelOp::gt>(lit<">">)) +
                   assign<&ast::CompareCond::right>(ref<Expression>))> {};

  struct Statement;

  struct AssignStmt
      : rule<assign<&ast::AssignStmt::name>(ref<ID>) + lit<":="> +
             assign<&ast::AssignStmt::value>(ref<Expression>)> {};

  struct CallStmt
      : rule<ikw<"CALL"> + assign<&ast::CallStmt::name>(ref<ID>)> {};

  struct WriteStmt
      : rule<ikw<"WRITE"> + assign<&ast::WriteStmt::name>(ref<ID>)> {};

  struct ReadStmt
      : rule<lit<"?"> + assign<&ast::ReadStmt::name>(ref<ID>)> {};

  struct PrintStmt
      : rule<lit<"!"> + assign<&ast::PrintStmt::value>(ref<Expression>)> {};

  struct CompoundStmt
      : rule<ikw<"BEGIN"> +
             some(append<&ast::CompoundStmt::statements>(ref<Statement>),
                  lit<";">) +
             ikw<"END">> {};

  struct IfStmt
      : rule<ikw<"IF"> + assign<&ast::IfStmt::condition>(ref<Condition>) +
             ikw<"THEN"> + assign<&ast::IfStmt::then>(ref<Statement>)> {};

  struct WhileStmt
      : rule<ikw<"WHILE"> +
             assign<&ast::WhileStmt::condition>(ref<Condition>) +
             ikw<"DO"> + assign<&ast::WhileStmt::body>(ref<Statement>)> {};

  struct EmptyStmt : rule<create<ast::EmptyStmt>> {};

  struct Statement : rule<ref<AssignStmt>   //
                          | ref<CallStmt>   //
                          | ref<WriteStmt>  //
                          | ref<ReadStmt>   //
                          | ref<PrintStmt>  //
                          | ref<CompoundStmt>  //
                          | ref<IfStmt>        //
                          | ref<WhileStmt>     //
                          | ref<EmptyStmt>> {};

  struct Block;

  struct Procedure
      : rule<ikw<"PROCEDURE"> + assign<&ast::Procedure::name>(ref<ID>) +
             lit<";"> + assign<&ast::Procedure::body>(ref<Block>) +
             lit<";">> {};

  struct ConstDecl
      : rule<assign<&ast::ConstDecl::name>(ref<ID>) + lit<"="> +
             assign<&ast::ConstDecl::value>(ref<NUMBER>)> {};

  struct Block
      : rule<option(ikw<"CONST"> +
                    some(append<&ast::Block::consts>(ref<ConstDecl>),
                         lit<",">) +
                    lit<";">) +
             option(ikw<"VAR"> +
                    some(append<&ast::Block::vars>(ref<ID>), lit<",">) +
                    lit<";">) +
             many(append<&ast::Block::procedures>(ref<Procedure>)) +
             assign<&ast::Block::body>(ref<Statement>)> {};

  struct Program
      : rule<assign<&ast::Program::block>(ref<Block>) + lit<".">> {};

  using entry = Program;
  using ignored = WS;
  static constexpr auto word_boundary = cr<"a-zA-Z">;
};

}  // namespace pl0
