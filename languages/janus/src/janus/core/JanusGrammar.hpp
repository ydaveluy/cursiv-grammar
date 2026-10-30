// The janus language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "janus/core/ast.hpp"

namespace janus {

inline constexpr auto upper_name = [](std::string_view text,
                                      std::pmr::memory_resource* memory) {
  if (text.find_first_of("abcdefghijklmnopqrstuvwxyz") == text.npos) return text;
  const auto room = cursiv::text_room(memory, text.size());
  for (std::size_t i = 0; i < text.size(); ++i)
    room[i] = text[i] >= 'a' && text[i] <= 'z'
                  ? static_cast<char>(text[i] - 'a' + 'A') : text[i];
  return std::string_view(room.data(), room.size());
};

struct Janus : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto Reserved = ikw<"PROCEDURE"> | ikw<"IF"> | ikw<"THEN"> |
      ikw<"ELSE"> | ikw<"FI"> | ikw<"FROM"> | ikw<"DO"> | ikw<"LOOP"> |
      ikw<"UNTIL"> | ikw<"CALL"> | ikw<"UNCALL"> | ikw<"READ"> | ikw<"WRITE">;
  // The original lexer folds ASCII case. Digit suffixes are a port extension.
  struct IDENT : terminal<as<upper_name>(!Reserved + cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9">))> {};
  struct NUM : terminal<as<std::int64_t>(some(d))> {};

  static constexpr auto BinOpTok =
      as<ast::BinOp::plus>(lit<"+">) | as<ast::BinOp::minus>(lit<"-">) | as<ast::BinOp::bang>(lit<"!">) | as<ast::BinOp::less_equal>(lit<"<=">) | as<ast::BinOp::less>(lit<"<">) |
      as<ast::BinOp::greater_equal>(lit<">=">) | as<ast::BinOp::greater>(lit<">">) | as<ast::BinOp::amp>(lit<"&">) | as<ast::BinOp::pipe>(lit<"|">) | as<ast::BinOp::equal>(lit<"=">) |
      as<ast::BinOp::hash>(lit<"#">) | as<ast::BinOp::star>(lit<"*">) | as<ast::BinOp::slash>(lit<"/">) | as<ast::BinOp::backslash>(lit<"\\">);

  static constexpr auto UnaryOpTok = as<ast::UnaryOp::negate>(lit<"-">) | as<ast::UnaryOp::complement>(lit<"~">);

  static constexpr auto ModOpTok =
      as<ast::ModOp::plus_eq>(lit<"+=">) | as<ast::ModOp::minus_eq>(lit<"-=">) | as<ast::ModOp::bang_eq>(lit<"!=">) | as<ast::ModOp::exchange>(lit<"<=>">);

  struct CALL : terminal<as<ast::CallKind::call>(ikw<"CALL">)> {};
  struct UNCALL : terminal<as<ast::CallKind::uncall>(ikw<"UNCALL">)> {};

  struct Expression;
  struct Lvalue;
  struct MinExp;

  // Precedence table with one level: expression: minexp | minexp BINOP
  // expression is right-recursive, so every operator is right-associative
  // and none binds tighter than another.
  struct Expression
      : infix<MinExp, ast::BinaryExpr, right(BinOpTok)> {};

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

  struct Statement;

  struct IfStmt
      : rule<ikw<"IF"> + assign<&ast::IfStmt::test>(ref<Expression>) +
             option(ikw<"THEN"> +
                    many(append<&ast::IfStmt::thenStatements>(
                        ref<Statement>))) +
             option(ikw<"ELSE"> +
                    many(append<&ast::IfStmt::elseStatements>(
                        ref<Statement>))) +
             ikw<"FI"> + assign<&ast::IfStmt::assertion>(ref<Expression>)> {};

  struct DoStmt
      : rule<ikw<"FROM"> + assign<&ast::DoStmt::from>(ref<Expression>) +
             option(ikw<"DO"> + many(append<&ast::DoStmt::doStatements>(
                                    ref<Statement>))) +
             option(ikw<"LOOP"> + many(append<&ast::DoStmt::loopStatements>(
                                      ref<Statement>))) +
             ikw<"UNTIL"> + assign<&ast::DoStmt::until>(ref<Expression>)> {};

  struct CallStmt
      : rule<create<ast::CallStmt> +
             assign<&ast::CallStmt::kind>(ref<CALL> | ref<UNCALL>) +
             assign<&ast::CallStmt::target>(ref<IDENT>)> {};

  struct ReadStmt
      : rule<ikw<"READ"> + assign<&ast::ReadStmt::name>(ref<IDENT>) +
             option(lit<"["> + assign<&ast::ReadStmt::index>(ref<Expression>) + lit<"]">)> {};

  struct WriteStmt
      : rule<ikw<"WRITE"> + assign<&ast::WriteStmt::name>(ref<IDENT>)> {};

  struct LvalStmt
      : rule<create<ast::ModStmt>(
                 assign<&ast::ModStmt::target>(ref<Lvalue>) +
                 assign<&ast::ModStmt::op>(ModOpTok) +
                 assign<&ast::ModStmt::value>(ref<Expression>)) |
             create<ast::SwapStmt>(
                 assign<&ast::SwapStmt::target>(ref<Lvalue>) + lit<":"> +
                 assign<&ast::SwapStmt::other>(ref<Lvalue>))> {};

  struct Statement
      : rule<ref<IfStmt> | ref<DoStmt> | ref<CallStmt> |
             ref<ReadStmt> | ref<WriteStmt> | ref<LvalStmt>> {};

  struct Procedure
      : rule<ikw<"PROCEDURE"> + assign<&ast::Procedure::name>(ref<IDENT>) +
             many(
                 append<&ast::Procedure::statements>(ref<Statement>))> {};

  struct VarDecl
      : rule<assign<&ast::VarDecl::name>(ref<IDENT>) +
             option(lit<"["> + assign<&ast::VarDecl::size>(ref<NUM>) +
                    lit<"]">)> {};

  struct Program
      : rule<create<ast::Program> +
             many(append<&ast::Program::decls>(ref<VarDecl>)) +
             many(append<&ast::Program::procedures>(ref<Procedure>))> {};

  using entry = Program;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace janus
