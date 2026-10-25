// The pl0 language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pl0/core/ast.hpp"

namespace pl0 {

struct Pl0 : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct ID : terminal<some(cr<"a-zA-Z">)> {};
  struct NUMBER : terminal<some(d), std::uint64_t> {};

  struct OP_ADD : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct OP_SUB : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct OP_MUL : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct OP_DIV : terminal<lit<"/">, value<ast::Operator::divide>> {};

  struct REL_EQ : terminal<lit<"=">, value<ast::RelOp::eq>> {};
  struct REL_NE : terminal<lit<"#">, value<ast::RelOp::ne>> {};
  struct REL_LE : terminal<lit<"<=">, value<ast::RelOp::le>> {};
  struct REL_LT : terminal<lit<"<">, value<ast::RelOp::lt>> {};
  struct REL_GE : terminal<lit<">=">, value<ast::RelOp::ge>> {};
  struct REL_GT : terminal<lit<">">, value<ast::RelOp::gt>> {};

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

  struct Term : infix<Factor, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
                      &ast::BinaryExpr::right,
                      left(ref<OP_MUL> | ref<OP_DIV>)> {};

  // A left-recursive chain, kept apart from `Term`'s `infix<>` table because
  // the sign binds to the first term only: `-1+2` is `(-1)+2`, not
  // `-(1+2)`.
  struct Expression
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<OP_ADD> | ref<OP_SUB>) +
                 assign<&ast::UnaryExpr::operand>(ref<Term>))  //
             | ref<Term>                                       //
             | create<ast::BinaryExpr>(
                   assign<&ast::BinaryExpr::left>(ref<Expression>) +
                   assign<&ast::BinaryExpr::op>(ref<OP_ADD> | ref<OP_SUB>) +
                   assign<&ast::BinaryExpr::right>(ref<Term>))> {};

  struct Condition
      : rule<create<ast::OddCond>(
                 ikw<"ODD"> +
                 assign<&ast::OddCond::operand>(ref<Expression>))  //
             | create<ast::CompareCond>(
                   assign<&ast::CompareCond::left>(ref<Expression>) +
                   assign<&ast::CompareCond::op>(
                       ref<REL_LE> | ref<REL_GE> | ref<REL_NE> |
                       ref<REL_EQ> | ref<REL_LT> | ref<REL_GT>) +
                   assign<&ast::CompareCond::right>(ref<Expression>))> {};

  struct Statement;

  struct AssignStmtRule
      : rule<assign<&ast::AssignStmt::name>(ref<ID>) + lit<":="> +
             assign<&ast::AssignStmt::value>(ref<Expression>)> {};

  struct CallStmtRule
      : rule<ikw<"CALL"> + assign<&ast::CallStmt::name>(ref<ID>)> {};

  struct WriteStmtRule
      : rule<ikw<"WRITE"> + assign<&ast::WriteStmt::name>(ref<ID>)> {};

  struct ReadStmtRule
      : rule<lit<"?"> + assign<&ast::ReadStmt::name>(ref<ID>)> {};

  struct PrintStmtRule
      : rule<lit<"!"> + assign<&ast::PrintStmt::value>(ref<Expression>)> {};

  struct CompoundStmtRule
      : rule<ikw<"BEGIN"> +
             some(append<&ast::CompoundStmt::statements>(ref<Statement>),
                  lit<";">) +
             ikw<"END">> {};

  struct IfStmtRule
      : rule<ikw<"IF"> + assign<&ast::IfStmt::condition>(ref<Condition>) +
             ikw<"THEN"> + assign<&ast::IfStmt::then>(ref<Statement>)> {};

  struct WhileStmtRule
      : rule<ikw<"WHILE"> +
             assign<&ast::WhileStmt::condition>(ref<Condition>) +
             ikw<"DO"> + assign<&ast::WhileStmt::body>(ref<Statement>)> {};

  struct EmptyStmtRule : rule<create<ast::EmptyStmt>> {};

  struct Statement : rule<ref<AssignStmtRule>   //
                          | ref<CallStmtRule>   //
                          | ref<WriteStmtRule>  //
                          | ref<ReadStmtRule>   //
                          | ref<PrintStmtRule>  //
                          | ref<CompoundStmtRule>  //
                          | ref<IfStmtRule>        //
                          | ref<WhileStmtRule>     //
                          | ref<EmptyStmtRule>> {};

  struct BlockRule;

  struct ProcedureRule
      : rule<ikw<"PROCEDURE"> + assign<&ast::Procedure::name>(ref<ID>) +
             lit<";"> + assign<&ast::Procedure::body>(ref<BlockRule>) +
             lit<";">> {};

  struct ConstDeclRule
      : rule<assign<&ast::ConstDecl::name>(ref<ID>) + lit<"="> +
             assign<&ast::ConstDecl::value>(ref<NUMBER>)> {};

  struct BlockRule
      : rule<option(ikw<"CONST"> +
                    some(append<&ast::Block::consts>(ref<ConstDeclRule>),
                         lit<",">) +
                    lit<";">) +
             option(ikw<"VAR"> +
                    some(append<&ast::Block::vars>(ref<ID>), lit<",">) +
                    lit<";">) +
             many(append<&ast::Block::procedures>(ref<ProcedureRule>)) +
             assign<&ast::Block::body>(ref<Statement>)> {};

  struct ProgramRule
      : rule<assign<&ast::Program::block>(ref<BlockRule>) + lit<".">> {};

  using entry = ProgramRule;
  using ignore = WS;
  static constexpr auto word_boundary = cr<"a-zA-Z">;
};

}  // namespace pl0
