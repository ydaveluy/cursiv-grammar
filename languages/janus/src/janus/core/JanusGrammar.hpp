// The janus language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "janus/core/ast.hpp"

namespace janus {

struct Janus : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  // no underscore: the janus lexer admits only letters and digits.
  struct IDENT : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9">)> {};
  // NUM is ONE digit, not a run of them: a two-digit constant has no
  // production, so it is not part of the language.
  struct NUM : terminal<cr<"0-9">, std::int32_t> {};

  struct PLUS : terminal<lit<"+">, value<ast::BinOp::plus>> {};
  struct MINUS : terminal<lit<"-">, value<ast::BinOp::minus>> {};
  struct BANG : terminal<lit<"!">, value<ast::BinOp::bang>> {};
  struct LESS_EQ : terminal<lit<"<=">, value<ast::BinOp::less_equal>> {};
  struct LESS : terminal<lit<"<">, value<ast::BinOp::less>> {};
  struct GREATER_EQ
      : terminal<lit<">=">, value<ast::BinOp::greater_equal>> {};
  struct GREATER : terminal<lit<">">, value<ast::BinOp::greater>> {};
  struct AMP : terminal<lit<"&">, value<ast::BinOp::amp>> {};
  struct PIPE : terminal<lit<"|">, value<ast::BinOp::pipe>> {};
  struct EQ : terminal<lit<"=">, value<ast::BinOp::equal>> {};
  struct HASH : terminal<lit<"#">, value<ast::BinOp::hash>> {};
  struct STAR : terminal<lit<"*">, value<ast::BinOp::star>> {};
  struct SLASH : terminal<lit<"/">, value<ast::BinOp::slash>> {};
  struct BACKSLASH : terminal<lit<"\\">, value<ast::BinOp::backslash>> {};

  static constexpr auto BinOpTok =
      ref<PLUS> | ref<MINUS> | ref<BANG> | ref<LESS_EQ> | ref<LESS> |
      ref<GREATER_EQ> | ref<GREATER> | ref<AMP> | ref<PIPE> | ref<EQ> |
      ref<HASH> | ref<STAR> | ref<SLASH> | ref<BACKSLASH>;

  struct NEG : terminal<lit<"-">, value<ast::UnaryOp::negate>> {};
  struct COMPL : terminal<lit<"~">, value<ast::UnaryOp::complement>> {};
  static constexpr auto UnaryOpTok = ref<NEG> | ref<COMPL>;

  struct PLUS_EQ : terminal<lit<"+=">, value<ast::ModOp::plus_eq>> {};
  struct MINUS_EQ : terminal<lit<"-=">, value<ast::ModOp::minus_eq>> {};
  struct BANG_EQ : terminal<lit<"!=">, value<ast::ModOp::bang_eq>> {};
  struct EXCHANGE : terminal<lit<"<=>">, value<ast::ModOp::exchange>> {};
  static constexpr auto ModOpTok =
      ref<PLUS_EQ> | ref<MINUS_EQ> | ref<BANG_EQ> | ref<EXCHANGE>;

  struct CALL : terminal<kw<"CALL">, value<ast::CallKind::call>> {};
  struct UNCALL : terminal<kw<"UNCALL">, value<ast::CallKind::uncall>> {};

  struct Expression;
  struct Lvalue;
  struct MinExp;

  // Precedence table with one level: expression: minexp | minexp BINOP
  // expression is right-recursive, so every operator is right-associative
  // and none binds tighter than another.
  struct Expression
      : infix<MinExp, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, right(BinOpTok)> {};

  struct Lvalue
      : rule<assign<&ast::Lvalue::name>(ref<IDENT>) +
             option(lit<"["> + assign<&ast::Lvalue::index>(ref<Expression>) +
                    lit<"]">)> {};

  struct MinExp
      : rule<create<ast::ParenExpr>(
                 lit<"("> +
                 assign<&ast::ParenExpr::expression>(ref<Expression>) +
                 lit<")">) |
             create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(UnaryOpTok) +
                 assign<&ast::UnaryExpr::operand>(ref<Expression>)) |
             ref<Lvalue> |
             create<ast::ConstantExpr>(
                 assign<&ast::ConstantExpr::value>(ref<NUM>))> {};

  struct StatementRule;

  struct IfStmtRule
      : rule<kw<"IF"> + assign<&ast::IfStmt::test>(ref<Expression>) +
             option(kw<"THEN"> +
                    some(append<&ast::IfStmt::thenStatements>(
                        ref<StatementRule>))) +
             option(kw<"ELSE"> +
                    some(append<&ast::IfStmt::elseStatements>(
                        ref<StatementRule>))) +
             kw<"FI"> + assign<&ast::IfStmt::assertion>(ref<Expression>)> {};

  struct DoStmtRule
      : rule<kw<"FROM"> + assign<&ast::DoStmt::from>(ref<Expression>) +
             option(kw<"DO"> + some(append<&ast::DoStmt::doStatements>(
                                    ref<StatementRule>))) +
             option(kw<"LOOP"> + some(append<&ast::DoStmt::loopStatements>(
                                      ref<StatementRule>))) +
             kw<"UNTIL"> + assign<&ast::DoStmt::until>(ref<Expression>)> {};

  struct CallStmtRule
      : rule<create<ast::CallStmt> +
             assign<&ast::CallStmt::kind>(ref<CALL> | ref<UNCALL>) +
             assign<&ast::CallStmt::target>(ref<IDENT>)> {};

  struct ReadStmtRule
      : rule<kw<"READ"> + assign<&ast::ReadStmt::name>(ref<IDENT>)> {};

  struct WriteStmtRule
      : rule<kw<"WRITE"> + assign<&ast::WriteStmt::name>(ref<IDENT>)> {};

  struct LvalStmtRule
      : rule<create<ast::ModStmt>(
                 assign<&ast::ModStmt::target>(ref<Lvalue>) +
                 assign<&ast::ModStmt::op>(ModOpTok) +
                 assign<&ast::ModStmt::value>(ref<Expression>)) |
             create<ast::SwapStmt>(
                 assign<&ast::SwapStmt::target>(ref<Lvalue>) + lit<":"> +
                 assign<&ast::SwapStmt::other>(ref<Lvalue>))> {};

  struct StatementRule
      : rule<ref<IfStmtRule> | ref<DoStmtRule> | ref<CallStmtRule> |
             ref<ReadStmtRule> | ref<WriteStmtRule> | ref<LvalStmtRule>> {};

  struct ProcedureRule
      : rule<kw<"PROCEDURE"> + assign<&ast::Procedure::name>(ref<IDENT>) +
             some(
                 append<&ast::Procedure::statements>(ref<StatementRule>))> {};

  struct VarDeclRule
      : rule<assign<&ast::VarDecl::name>(ref<IDENT>) +
             option(lit<"["> + assign<&ast::VarDecl::size>(ref<NUM>) +
                    lit<"]">)> {};

  struct ProgramRule
      : rule<create<ast::Program> +
             many(append<&ast::Program::decls>(ref<VarDeclRule>)) +
             many(append<&ast::Program::procedures>(ref<ProcedureRule>))> {};

  using entry = ProgramRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace janus
