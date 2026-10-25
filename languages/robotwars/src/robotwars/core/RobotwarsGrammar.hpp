// The robotwars language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "robotwars/core/ast.hpp"

namespace robotwars {

struct Robotwars : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct EOL : terminal<some(cr<R"(\r\n)">)> {};

  // A comment is ordinary line content here, not trivia: it is one of the
  // three things a `line` may hold, and the AST keeps its text.
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z"> + some(cr<"a-zA-Z0-9">)> {};

  // Unsigned: `+`/`-` are the ADD/SUB tokens below, never fused with the
  // digits that follow -- `number`'s optional sign is a parser-level rule.
  struct NUMBER : terminal<some(d), std::int64_t> {};

  struct DOT : terminal<lit<".">> {};
  struct COMMA : terminal<lit<",">> {};

  struct ADD : terminal<lit<"+">, value<ast::Operation::add>> {};
  struct SUB : terminal<lit<"-">, value<ast::Operation::subtract>> {};
  struct MUL : terminal<lit<"*">, value<ast::Operation::multiply>> {};
  struct DIV : terminal<lit<"/">, value<ast::Operation::divide>> {};

  struct LESS : terminal<lit<"<">, value<ast::Comparison::less>> {};
  struct GREATER : terminal<lit<">">, value<ast::Comparison::greater>> {};
  struct EQUAL : terminal<lit<"=">, value<ast::Comparison::equal>> {};
  struct NOTEQUAL : terminal<lit<"#">, value<ast::Comparison::notEqual>> {};

  static constexpr auto OpArith = ref<ADD> | ref<SUB> | ref<MUL> | ref<DIV>;
  static constexpr auto OpCompare =
      ref<LESS> | ref<GREATER> | ref<EQUAL> | ref<NOTEQUAL>;

  // The 26 single-letter registers. Named `REG_x`: a bare `S`, `W` or `D`
  // would hide the character class GrammarSpec already gives that name.
  struct REG_A : terminal<kw<"A">, value<ast::Register::a>> {};
  struct REG_B : terminal<kw<"B">, value<ast::Register::b>> {};
  struct REG_C : terminal<kw<"C">, value<ast::Register::c>> {};
  struct REG_D : terminal<kw<"D">, value<ast::Register::d>> {};
  struct REG_E : terminal<kw<"E">, value<ast::Register::e>> {};
  struct REG_F : terminal<kw<"F">, value<ast::Register::f>> {};
  struct REG_G : terminal<kw<"G">, value<ast::Register::g>> {};
  struct REG_H : terminal<kw<"H">, value<ast::Register::h>> {};
  struct REG_I : terminal<kw<"I">, value<ast::Register::i>> {};
  struct REG_J : terminal<kw<"J">, value<ast::Register::j>> {};
  struct REG_K : terminal<kw<"K">, value<ast::Register::k>> {};
  struct REG_L : terminal<kw<"L">, value<ast::Register::l>> {};
  struct REG_M : terminal<kw<"M">, value<ast::Register::m>> {};
  struct REG_N : terminal<kw<"N">, value<ast::Register::n>> {};
  struct REG_O : terminal<kw<"O">, value<ast::Register::o>> {};
  struct REG_P : terminal<kw<"P">, value<ast::Register::p>> {};
  struct REG_Q : terminal<kw<"Q">, value<ast::Register::q>> {};
  struct REG_R : terminal<kw<"R">, value<ast::Register::r>> {};
  struct REG_S : terminal<kw<"S">, value<ast::Register::s>> {};
  struct REG_T : terminal<kw<"T">, value<ast::Register::t>> {};
  struct REG_U : terminal<kw<"U">, value<ast::Register::u>> {};
  struct REG_V : terminal<kw<"V">, value<ast::Register::v>> {};
  struct REG_W : terminal<kw<"W">, value<ast::Register::w>> {};
  struct REG_X : terminal<kw<"X">, value<ast::Register::x>> {};
  struct REG_Y : terminal<kw<"Y">, value<ast::Register::y>> {};
  struct REG_Z : terminal<kw<"Z">, value<ast::Register::z>> {};

  struct AIM : terminal<kw<"AIM">, value<ast::Register::aim>> {};
  struct SHOT : terminal<kw<"SHOT">, value<ast::Register::shot>> {};
  struct RADAR : terminal<kw<"RADAR">, value<ast::Register::radar>> {};
  struct DAMAGE : terminal<kw<"DAMAGE">, value<ast::Register::damage>> {};
  struct SPEEDX : terminal<kw<"SPEEDX">, value<ast::Register::speedx>> {};
  struct SPEEDY : terminal<kw<"SPEEDY">, value<ast::Register::speedy>> {};
  struct RANDOM : terminal<kw<"RANDOM">, value<ast::Register::random>> {};
  struct INDEX : terminal<kw<"INDEX">, value<ast::Register::index>> {};
  struct DATA : terminal<kw<"DATA">, value<ast::Register::data>> {};

  // An operand's `register | DATA` reads `register` alone: DATA is already
  // the last of its 35 alternatives, so a second mention would just be the
  // same token spelled twice.
  static constexpr auto AnyRegister =
      ref<REG_A> | ref<REG_B> | ref<REG_C> | ref<REG_D> | ref<REG_E> |
      ref<REG_F> | ref<REG_G> | ref<REG_H> | ref<REG_I> | ref<REG_J> |
      ref<REG_K> | ref<REG_L> | ref<REG_M> | ref<REG_N> | ref<REG_O> |
      ref<REG_P> | ref<REG_Q> | ref<REG_R> | ref<REG_S> | ref<REG_T> |
      ref<REG_U> | ref<REG_V> | ref<REG_W> | ref<REG_X> | ref<REG_Y> |
      ref<REG_Z> | ref<AIM> | ref<SHOT> | ref<RADAR> | ref<DAMAGE> |
      ref<SPEEDX> | ref<SPEEDY> | ref<RANDOM> | ref<INDEX> | ref<DATA>;

  struct Statement;

  // A number's optional sign is a parser-level rule: the lexer keeps the
  // sign and the digits as separate tokens.
  struct NumberLitRule
      : rule<option(flag<&ast::NumberLit::negative>(ref<SUB>) |
                    ref<ADD>) +
            assign<&ast::NumberLit::value>(ref<NUMBER>)> {};
  struct RegisterRefRule
      : rule<assign<&ast::RegisterRef::reg>(AnyRegister)> {};

  // A leaf never opens on `*`/`/`, and its own sign already reads a leading
  // `+`/`-` in front of a number -- so it never competes with `UnaryRule`
  // below for the token that starts an expression.
  struct Leaf : rule<ref<NumberLitRule> | ref<RegisterRefRule>> {};

  // One flat, precedence-free level: an operand, then any run of
  // `operator operand`.
  struct Chain
      : infix<Leaf, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
             &ast::BinaryExpr::right, (left(OpArith))> {};

  // A leading operator with no left operand, and no continuation of its
  // own -- only `Chain` repeats. `+`/`-` before a number is
  // `NumberLitRule`'s own sign (above), so this takes a register there;
  // `*`/`/` have no such rule to collide with and take either leaf.
  struct UnaryRule
      : rule<create<ast::UnaryExpr>(
            (assign<&ast::UnaryExpr::op>(ref<MUL> | ref<DIV>) +
             assign<&ast::UnaryExpr::operand>(ref<Leaf>)) |
            (assign<&ast::UnaryExpr::op>(ref<ADD> | ref<SUB>) +
             assign<&ast::UnaryExpr::operand>(ref<RegisterRefRule>)))> {};

  struct Expression : rule<ref<UnaryRule> | ref<Chain>> {};

  struct ConditionRule
      : rule<assign<&ast::Condition::left>(ref<Expression>) +
             assign<&ast::Condition::op>(OpCompare) +
             assign<&ast::Condition::right>(ref<Expression>)> {};

  struct LabelRule : rule<assign<&ast::Label::name>(ref<ID>)> {};
  struct CommentRule : rule<assign<&ast::Comment::text>(ref<COMMENT>)> {};

  struct IfStatementRule
      : rule<option(kw<"IF">) +
             assign<&ast::IfStatement::condition>(ref<ConditionRule>) +
             option(ref<EOL> | ref<COMMA> | ref<DOT>) +
             assign<&ast::IfStatement::body>(ref<Statement>)> {};

  struct ToStatementRule
      : rule<option(assign<&ast::ToStatement::value>(ref<Expression>)) +
             some(kw<"TO"> +
                 append<&ast::ToStatement::targets>(AnyRegister))> {};

  struct GosubStatementRule
      : rule<kw<"GOSUB"> + assign<&ast::GosubStatement::target>(ref<ID>)> {};

  struct GotoStatementRule
      : rule<kw<"GOTO"> + assign<&ast::GotoStatement::target>(ref<ID>)> {};

  struct EndSubStatementRule
      : rule<create<ast::EndSubStatement> + kw<"ENDSUB">> {};

  // `=` and `#` read no expression, `<`/`>` do.
  struct AccumStatementRule
      : rule<(assign<&ast::AccumStatement::op>(ref<EQUAL>) +
             assign<&ast::AccumStatement::body>(ref<Statement>)) |
            (assign<&ast::AccumStatement::op>(ref<NOTEQUAL>) +
             assign<&ast::AccumStatement::body>(ref<Statement>)) |
            (assign<&ast::AccumStatement::op>(ref<LESS>) +
             assign<&ast::AccumStatement::operand>(ref<Expression>) +
             assign<&ast::AccumStatement::body>(ref<Statement>)) |
            (assign<&ast::AccumStatement::op>(ref<GREATER>) +
             assign<&ast::AccumStatement::operand>(ref<Expression>) +
             assign<&ast::AccumStatement::body>(ref<Statement>))> {};

  struct Statement
      : rule<ref<IfStatementRule> | ref<ToStatementRule> |
            ref<GosubStatementRule> | ref<GotoStatementRule> |
            ref<EndSubStatementRule> | ref<AccumStatementRule>> {};

  // A program is lines to EOF; a line is inlined here, holding at most one
  // item, then the EOL that closes it.
  struct ProgramRule
      : rule<create<ast::Program> +
            some(option(append<&ast::Program::items>(ref<LabelRule>) |
                       append<&ast::Program::items>(ref<CommentRule>) |
                       append<&ast::Program::items>(ref<Statement>)) +
                ref<EOL>) +
            eof> {};

  using entry = ProgramRule;
  using ignore = WS;
};

}  // namespace robotwars
