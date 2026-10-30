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
  struct NUMBER : terminal<as<std::int64_t>(some(d))> {};

  struct DOT : terminal<lit<".">> {};
  struct COMMA : terminal<lit<",">> {};

  struct ADD : terminal<as<ast::Operation::add>(lit<"+">)> {};
  struct SUB : terminal<as<ast::Operation::subtract>(lit<"-">)> {};

  static constexpr auto OpArith = ref<ADD> | ref<SUB> | as<ast::Operation::multiply>(lit<"*">) | as<ast::Operation::divide>(lit<"/">);
  static constexpr auto OpCompare =
      as<ast::Comparison::less>(lit<"<">) | as<ast::Comparison::greater>(lit<">">) | as<ast::Comparison::equal>(lit<"=">) | as<ast::Comparison::notEqual>(lit<"#">);

  // The 26 single-letter registers. Named `REG_x`: a bare `S`, `W` or `D`
  // would hide the character class GrammarSpec already gives that name.

  struct AIM : terminal<as<ast::Register::aim>(ikw<"AIM">)> {};
  struct SHOT : terminal<as<ast::Register::shot>(ikw<"SHOT">)> {};
  struct RADAR : terminal<as<ast::Register::radar>(ikw<"RADAR">)> {};
  struct DAMAGE : terminal<as<ast::Register::damage>(ikw<"DAMAGE">)> {};
  struct SPEEDX : terminal<as<ast::Register::speedx>(ikw<"SPEEDX">)> {};
  struct SPEEDY : terminal<as<ast::Register::speedy>(ikw<"SPEEDY">)> {};
  struct RANDOM : terminal<as<ast::Register::random>(ikw<"RANDOM">)> {};
  struct INDEX : terminal<as<ast::Register::index>(ikw<"INDEX">)> {};
  struct DATA : terminal<as<ast::Register::data>(ikw<"DATA">)> {};

  // An operand's `register | DATA` reads `register` alone: DATA is already
  // the last of its 35 alternatives, so a second mention would just be the
  // same token spelled twice.
  static constexpr auto AnyRegister =
      as<ast::Register::a>(ikw<"A">) | as<ast::Register::b>(ikw<"B">) | as<ast::Register::c>(ikw<"C">) | as<ast::Register::d>(ikw<"D">) | as<ast::Register::e>(ikw<"E">) |
      as<ast::Register::f>(ikw<"F">) | as<ast::Register::g>(ikw<"G">) | as<ast::Register::h>(ikw<"H">) | as<ast::Register::i>(ikw<"I">) | as<ast::Register::j>(ikw<"J">) |
      as<ast::Register::k>(ikw<"K">) | as<ast::Register::l>(ikw<"L">) | as<ast::Register::m>(ikw<"M">) | as<ast::Register::n>(ikw<"N">) | as<ast::Register::o>(ikw<"O">) |
      as<ast::Register::p>(ikw<"P">) | as<ast::Register::q>(ikw<"Q">) | as<ast::Register::r>(ikw<"R">) | as<ast::Register::s>(ikw<"S">) | as<ast::Register::t>(ikw<"T">) |
      as<ast::Register::u>(ikw<"U">) | as<ast::Register::v>(ikw<"V">) | as<ast::Register::w>(ikw<"W">) | as<ast::Register::x>(ikw<"X">) | as<ast::Register::y>(ikw<"Y">) |
      as<ast::Register::z>(ikw<"Z">) | ref<AIM> | ref<SHOT> | ref<RADAR> | ref<DAMAGE> |
      ref<SPEEDX> | ref<SPEEDY> | ref<RANDOM> | ref<INDEX> | ref<DATA>;

  struct Statement;

  // A number's optional sign is a parser-level rule: the lexer keeps the
  // sign and the digits as separate tokens.
  struct NumberLit
      : rule<option(flag<&ast::NumberLit::negative>(ref<SUB>) |
                    ref<ADD>) +
            assign<&ast::NumberLit::value>(ref<NUMBER>)> {};
  struct RegisterRef
      : rule<assign<&ast::RegisterRef::reg>(AnyRegister)> {};

  // A leaf never opens on `*`/`/`, and its own sign already reads a leading
  // `+`/`-` in front of a number -- so it never competes with `Unary`
  // below for the token that starts an expression.
  struct Leaf : rule<ref<NumberLit> | ref<RegisterRef>> {};

  // A leading operator reads the current accumulator as its implicit left
  // operand. It may begin the same left-to-right chain as an ordinary leaf.
  struct Unary
      : rule<create<ast::UnaryExpr>(
            (assign<&ast::UnaryExpr::op>(as<ast::Operation::multiply>(lit<"*">) | as<ast::Operation::divide>(lit<"/">)) +
             assign<&ast::UnaryExpr::operand>(ref<Leaf>)) |
            (assign<&ast::UnaryExpr::op>(ref<ADD> | ref<SUB>) +
             assign<&ast::UnaryExpr::operand>(ref<RegisterRef>)))> {};

  struct Operand : rule<ref<Unary> | ref<Leaf>> {};
  struct Expression
      : infix<Operand, ast::BinaryExpr, left(OpArith)> {};

  struct Condition
      : rule<assign<&ast::Condition::left>(ref<Expression>) +
             assign<&ast::Condition::op>(OpCompare) +
             assign<&ast::Condition::right>(ref<Expression>)> {};

  struct Label : rule<assign<&ast::Label::name>(ref<ID>)> {};
  struct Comment : rule<assign<&ast::Comment::text>(ref<COMMENT>)> {};

  struct IfStatement
      : rule<option(ikw<"IF">) +
             assign<&ast::IfStatement::condition>(ref<Condition>) +
             option(ref<EOL> | ref<COMMA> | ref<DOT>) +
             assign<&ast::IfStatement::body>(ref<Statement>)> {};

  struct ToStatement
      : rule<option(assign<&ast::ToStatement::value>(ref<Expression>)) +
             some(ikw<"TO"> +
                 append<&ast::ToStatement::targets>(AnyRegister))> {};

  struct GosubStatement
      : rule<ikw<"GOSUB"> + assign<&ast::GosubStatement::target>(ref<ID>)> {};

  struct GotoStatement
      : rule<ikw<"GOTO"> + assign<&ast::GotoStatement::target>(ref<ID>)> {};

  struct EndSubStatement
      : rule<create<ast::EndSubStatement> + ikw<"ENDSUB">> {};

  // `=` and `#` read no expression, `<`/`>` do.
  struct AccumStatement
      : rule<(assign<&ast::AccumStatement::op>(as<ast::Comparison::equal>(lit<"=">)) +
             assign<&ast::AccumStatement::body>(ref<Statement>)) |
            (assign<&ast::AccumStatement::op>(as<ast::Comparison::notEqual>(lit<"#">)) +
             assign<&ast::AccumStatement::body>(ref<Statement>)) |
            (assign<&ast::AccumStatement::op>(as<ast::Comparison::less>(lit<"<">)) +
             assign<&ast::AccumStatement::operand>(ref<Expression>) +
             assign<&ast::AccumStatement::body>(ref<Statement>)) |
            (assign<&ast::AccumStatement::op>(as<ast::Comparison::greater>(lit<">">)) +
             assign<&ast::AccumStatement::operand>(ref<Expression>) +
             assign<&ast::AccumStatement::body>(ref<Statement>))> {};

  struct Statement
      : rule<ref<IfStatement> | ref<ToStatement> |
            ref<GosubStatement> | ref<GotoStatement> |
            ref<EndSubStatement> | ref<AccumStatement>> {};

  // Comments are retained as ordered items, including after line content.
  struct Program
      : rule<create<ast::Program> +
            some(option(append<&ast::Program::items>(ref<Label>) |
                        append<&ast::Program::items>(ref<Statement>)) +
                 option(append<&ast::Program::items>(ref<Comment>)) + ref<EOL>) +
            eof> {};

  using entry = Program;
  using ignored = WS;
};

}  // namespace robotwars
