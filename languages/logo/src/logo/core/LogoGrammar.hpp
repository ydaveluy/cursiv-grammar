// The logo language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "logo/core/ast.hpp"

namespace logo {

inline constexpr auto drop_quote = [](std::string_view t) {
  return t.substr(1);
};

inline constexpr auto drop_brackets = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

struct Logo : cursiv::GrammarSpec {
  struct WS : terminal<cr<R"( \t)">> {};
  struct EOL : terminal<option(lit<"\r">) + lit<"\n">> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto name_body = cr<"a-zA-Z"> + many(w);
  static constexpr auto reserved = kw<"repeat"> | kw<"fd"> | kw<"forward"> |
      kw<"bk"> | kw<"backward"> | kw<"rt"> | kw<"right"> | kw<"lt"> | kw<"left"> |
      kw<"cs"> | kw<"clearscreen"> | kw<"pu"> | kw<"penup"> | kw<"pd"> | kw<"pendown"> |
      kw<"ht"> | kw<"hideturtle"> | kw<"st"> | kw<"showturtle"> | kw<"home"> |
      kw<"label"> | kw<"stop"> | kw<"setxy"> | kw<"make"> | kw<"if"> | kw<"for"> |
      kw<"print"> | kw<"random"> | kw<"to"> | kw<"end">;
  struct STRING : terminal<!reserved + name_body> {};
  struct STRINGLITERAL : terminal<as<drop_quote>(lit<"\""> + name_body)> {};
  struct NUMBER : terminal<some(d)> {};

  struct QuotedString
      : terminal<as<drop_brackets>(
            lit<"["> + many(ref<QuotedString> | (ref<COMMENT> + !cr<R"(^\r\n)">) |
                some(cr<R"( \t\r\n)">) | (lit<"\""> + name_body) |
                name_body | some(d) | cr<"+*/:<>=,-">) + lit<"]">)> {};

  struct Expression;
  struct Cmd;
  struct Line;

  // `random` is the only function, so it is inlined here rather than kept
  // as its own pass-through node.
  struct Number : rule<assign<&ast::NumberLiteral::value>(ref<NUMBER>)> {};
  struct Atom
      : rule<ref<Number> | create<ast::Deref>(lit<":"> +
                                   assign<&ast::Deref::name>(ref<STRING>))  //
             | create<ast::RandomCall>(
                   kw<"random"> +
                   assign<&ast::RandomCall::arg>(ref<Expression>))> {};

  // A sign-less atom forwards through with no wrapper node.
  struct SignExpression
      : rule<create<ast::SignExpression>(
                 some(append<&ast::SignExpression::signs>(as<ast::Sign::plus>(lit<"+">) |
                                                           as<ast::Sign::minus>(lit<"-">))) +
                 assign<&ast::SignExpression::operand>(ref<Atom>))  //
             | ref<Atom>> {};

  // Ordinary arithmetic precedence: multiply/divide tighter than add/subtract.
  struct Expression
      : infix<SignExpression, ast::BinaryExpression,
              levels(left(as<ast::Op::multiply>(lit<"*">) | as<ast::Op::divide>(lit<"/">)),
                     left(as<ast::Op::add>(lit<"+">) | as<ast::Op::subtract>(lit<"-">)))> {};

  // A bare deref is dropped as its own alternative: it already parses as an
  // expression (SignExpression -> Atom), so it would only fork the parse.
  static constexpr auto value = ref<STRINGLITERAL> | ref<Expression>;

  struct Cmd
      : rule<create<ast::RepeatCmd>(
                 kw<"repeat"> +
                 assign<&ast::RepeatCmd::count>(ref<Number>) + lit<"["> +
                 some(append<&ast::RepeatCmd::body>(ref<Cmd>)) +
                 lit<"]">)  //
             | create<ast::ForwardCmd>(
                   (kw<"fd"> | kw<"forward">) +
                   assign<&ast::ForwardCmd::distance>(ref<Expression>))  //
             | create<ast::BackwardCmd>(
                   (kw<"bk"> | kw<"backward">) +
                   assign<&ast::BackwardCmd::distance>(ref<Expression>))  //
             | create<ast::RightCmd>(
                   (kw<"rt"> | kw<"right">) +
                   assign<&ast::RightCmd::angle>(ref<Expression>))  //
             | create<ast::LeftCmd>(
                   (kw<"lt"> | kw<"left">) +
                   assign<&ast::LeftCmd::angle>(ref<Expression>))  //
             | create<ast::ClearScreenCmd>(kw<"cs"> | kw<"clearscreen">)  //
             | create<ast::PenUpCmd>(kw<"pu"> | kw<"penup">)              //
             | create<ast::PenDownCmd>(kw<"pd"> | kw<"pendown">)          //
             | create<ast::HideTurtleCmd>(kw<"ht"> | kw<"hideturtle">)    //
             | create<ast::ShowTurtleCmd>(kw<"st"> | kw<"showturtle">)    //
             | create<ast::HomeCmd>(kw<"home">)                          //
             | create<ast::LabelCmd>(kw<"label">)                        //
             | create<ast::SetxyCmd>(
                   kw<"setxy"> + assign<&ast::SetxyCmd::x>(ref<Expression>) +
                   assign<&ast::SetxyCmd::y>(ref<Expression>))  //
             | create<ast::MakeCmd>(
                   kw<"make"> +
                   assign<&ast::MakeCmd::name>(ref<STRINGLITERAL>) +
                   assign<&ast::MakeCmd::value>(value))  //
             | create<ast::ProcedureInvocation>(
                   assign<&ast::ProcedureInvocation::name>(ref<STRING>) +
                   many(append<&ast::ProcedureInvocation::args>(
                       ref<Expression>)))  //
             | create<ast::IfCmd>(
                   kw<"if"> + assign<&ast::IfCmd::left>(ref<Expression>) +
                   assign<&ast::IfCmd::op>(as<ast::CompareOp::less>(lit<"<">) | as<ast::CompareOp::greater>(lit<">">) |
                                            as<ast::CompareOp::equal>(lit<"=">)) +
                   assign<&ast::IfCmd::right>(ref<Expression>) + lit<"["> +
                   some(append<&ast::IfCmd::body>(ref<Cmd>)) +
                   lit<"]">)  //
             | create<ast::StopCmd>(kw<"stop">)  //
             | create<ast::ForCmd>(
                   kw<"for"> + lit<"["> +
                   assign<&ast::ForCmd::var>(ref<STRING>) +
                   assign<&ast::ForCmd::from>(ref<Expression>) +
                   assign<&ast::ForCmd::to>(ref<Expression>) +
                   assign<&ast::ForCmd::step>(ref<Expression>) + lit<"]"> +
                   lit<"["> +
                   some(append<&ast::ForCmd::body>(ref<Cmd>)) +
                   lit<"]">)> {};

  // A comma-joined run of `:name`s, itself repeated (space-separated) any
  // number of times. Splicing directly into the enclosing node avoids a
  // helper rule with a second head.
  static constexpr auto one_param =
      lit<":"> +
      append<&ast::ProcedureDeclaration::params>(ref<STRING>);
  static constexpr auto param_group = one_param + many(lit<","> + one_param);

  struct ProcedureDeclaration
      : rule<kw<"to"> +
             assign<&ast::ProcedureDeclaration::name>(ref<STRING>) +
             many(param_group) + option(ref<EOL>) +
             some(option(append<&ast::ProcedureDeclaration::body>(
                      ref<Line>)) +
                  ref<EOL>) +
             kw<"end">> {};

  struct Line
      : rule<create<ast::CommandLine>(
                 some(append<&ast::CommandLine::commands>(ref<Cmd>)) +
                 option(assign<&ast::CommandLine::comment>(ref<COMMENT>)))  //
             | create<ast::CommentLine>(
                   assign<&ast::CommentLine::text>(ref<COMMENT>))  //
             | create<ast::PrintLine>(
                   kw<"print"> +
                   assign<&ast::PrintLine::value>(
                       value | ref<QuotedString>) +
                   option(assign<&ast::PrintLine::comment>(ref<COMMENT>)))  //
             | ref<ProcedureDeclaration>> {};

  struct Program
      : rule<create<ast::Program>(
                 some(option(append<&ast::Program::lines>(ref<Line>)) +
                      ref<EOL>) +
                 option(append<&ast::Program::lines>(ref<Line>)))> {};

  using entry = Program;
  using ignored = WS;
};

}  // namespace logo
