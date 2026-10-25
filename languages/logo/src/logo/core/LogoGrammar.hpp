// The logo language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "logo/core/ast.hpp"

namespace logo {

inline constexpr auto drop_quote = [](std::string_view t) {
  return t.substr(1);
};
using Unquoted = cursiv::text_conv<drop_quote>;

inline constexpr auto drop_brackets = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unbracketed = cursiv::text_conv<drop_brackets>;

struct Logo : cursiv::GrammarSpec {
  struct WS : terminal<cr<R"( \t)">> {};
  struct EOL : terminal<option(lit<"\r">) + lit<"\n">> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto name_body = cr<"a-zA-Z"> + many(w);
  struct STRING : terminal<name_body> {};
  struct STRINGLITERAL : terminal<lit<"\""> + name_body, Unquoted> {};
  struct NUMBER : terminal<some(d), std::int64_t> {};

  struct QuotedString
      : terminal<lit<"["> +
                 many(ref<QuotedString> | (!lit<"["> + !lit<"]"> + any)) +
                 lit<"]">, Unbracketed> {};

  struct PLUS : terminal<lit<"+">, value<ast::Sign::plus>> {};
  struct MINUS : terminal<lit<"-">, value<ast::Sign::minus>> {};
  struct ADD : terminal<lit<"+">, value<ast::Op::add>> {};
  struct SUBTRACT : terminal<lit<"-">, value<ast::Op::subtract>> {};
  struct MULTIPLY : terminal<lit<"*">, value<ast::Op::multiply>> {};
  struct DIVIDE : terminal<lit<"/">, value<ast::Op::divide>> {};
  struct LESS : terminal<lit<"<">, value<ast::CompareOp::less>> {};
  struct GREATER : terminal<lit<">">, value<ast::CompareOp::greater>> {};
  struct EQUAL : terminal<lit<"=">, value<ast::CompareOp::equal>> {};

  struct Expression;
  struct CmdRule;
  struct LineRule;

  // `random` is the only function, so it is inlined here rather than kept
  // as its own pass-through node.
  struct AtomRule
      : rule<create<ast::NumberLiteral>(
                 assign<&ast::NumberLiteral::value>(ref<NUMBER>))  //
             | create<ast::Deref>(lit<":"> +
                                   assign<&ast::Deref::name>(ref<STRING>))  //
             | create<ast::RandomCall>(
                   kw<"random"> +
                   assign<&ast::RandomCall::arg>(ref<Expression>))> {};

  // A sign-less atom forwards through with no wrapper node.
  struct SignExpression
      : rule<create<ast::SignExpression>(
                 some(append<&ast::SignExpression::signs>(ref<PLUS> |
                                                           ref<MINUS>)) +
                 assign<&ast::SignExpression::operand>(ref<AtomRule>))  //
             | ref<AtomRule>> {};

  // Ordinary arithmetic precedence: multiply/divide tighter than add/subtract.
  struct Expression
      : infix<SignExpression, &ast::BinaryExpression::left,
              &ast::BinaryExpression::op, &ast::BinaryExpression::right,
              (left(ref<MULTIPLY> | ref<DIVIDE>) >>
               left(ref<ADD> | ref<SUBTRACT>))> {};

  // A bare deref is dropped as its own alternative: it already parses as an
  // expression (SignExpression -> AtomRule), so it would only fork the parse.
  static constexpr auto value = ref<STRINGLITERAL> | ref<Expression>;

  struct CmdRule
      : rule<create<ast::RepeatCmd>(
                 kw<"repeat"> +
                 assign<&ast::RepeatCmd::count>(ref<Expression>) + lit<"["> +
                 some(append<&ast::RepeatCmd::body>(ref<CmdRule>)) +
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
                   assign<&ast::IfCmd::op>(ref<LESS> | ref<GREATER> |
                                            ref<EQUAL>) +
                   assign<&ast::IfCmd::right>(ref<Expression>) + lit<"["> +
                   some(append<&ast::IfCmd::body>(ref<CmdRule>)) +
                   lit<"]">)  //
             | create<ast::StopCmd>(kw<"stop">)  //
             | create<ast::ForCmd>(
                   kw<"for"> + lit<"["> +
                   assign<&ast::ForCmd::var>(ref<STRING>) +
                   assign<&ast::ForCmd::from>(ref<Expression>) +
                   assign<&ast::ForCmd::to>(ref<Expression>) +
                   assign<&ast::ForCmd::step>(ref<Expression>) + lit<"]"> +
                   lit<"["> +
                   some(append<&ast::ForCmd::body>(ref<CmdRule>)) +
                   lit<"]">)> {};

  // A comma-joined run of `:name`s, itself repeated (space-separated) any
  // number of times. Splicing directly into the enclosing node avoids a
  // helper rule with a second head.
  static constexpr auto one_param =
      lit<":"> +
      append<&ast::ProcedureDeclaration::params>(ref<STRING>);
  static constexpr auto param_group = one_param + many(lit<","> + one_param);

  struct ProcedureDeclarationRule
      : rule<kw<"to"> +
             assign<&ast::ProcedureDeclaration::name>(ref<STRING>) +
             many(param_group) + option(ref<EOL>) +
             some(option(append<&ast::ProcedureDeclaration::body>(
                      ref<LineRule>)) +
                  ref<EOL>) +
             kw<"end">> {};

  struct LineRule
      : rule<create<ast::CommandLine>(
                 some(append<&ast::CommandLine::commands>(ref<CmdRule>)) +
                 option(assign<&ast::CommandLine::comment>(ref<COMMENT>)))  //
             | create<ast::CommentLine>(
                   assign<&ast::CommentLine::text>(ref<COMMENT>))  //
             | create<ast::PrintLine>(
                   kw<"print"> +
                   assign<&ast::PrintLine::value>(
                       value | ref<QuotedString>) +
                   option(assign<&ast::PrintLine::comment>(ref<COMMENT>)))  //
             | ref<ProcedureDeclarationRule>> {};

  struct ProgramRule
      : rule<create<ast::Program>(
                 some(option(append<&ast::Program::lines>(ref<LineRule>)) +
                      ref<EOL>) +
                 option(append<&ast::Program::lines>(ref<LineRule>)))> {};

  using entry = ProgramRule;
  using ignore = WS;
};

}  // namespace logo
