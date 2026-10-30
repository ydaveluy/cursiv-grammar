// The mumps language: its grammar.
#pragma once

#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "mumps/core/ast.hpp"

namespace mumps {

inline constexpr auto unquote = [](std::string_view t,
                                   std::pmr::memory_resource* mem) {
  const auto body = t.substr(1, t.size() - 2);
  if (body.find("\"\"") == std::string_view::npos) return body;
  auto room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[n++] = body[i];
    if (body[i] == '"' && i + 1 < body.size() && body[i + 1] == '"') ++i;
  }
  return std::string_view(room.data(), n);
};

struct Mumps : cursiv::GrammarSpec {
  struct TAB : terminal<lit<"\t">> {};
  struct SPACE : terminal<lit<" ">> {};
  struct NEWLINE : terminal<some(cr<R"(\r\n)">)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  // M command words end before a space/postcondition or line end. Keep
  // this byte guard in a terminal: contextual words like i are also native
  // variable names, and an unguarded keyword would win the lexical tie in i=.
  template <cursiv::str Name>
  struct CommandWord
      : terminal<ikw<Name> + &(cr<R"( :;\r\n)"> | eof)> {};

  struct ID : terminal<cr<"a-zA-Z%"> + many(cr<"a-zA-Z0-9">)> {};
  struct LABEL : terminal<ref<ID> | some(d)> {};
  struct ROUTINE_NAME
      : terminal<(cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9">)) | some(d)> {};
  struct PERCENT_ROUTINE_NAME
      : terminal<as<[](std::string_view text) { return text.substr(1); }>(
            lit<"%"> + many(cr<"a-zA-Z0-9">))> {};

  static constexpr auto stringBody = many(lit<"\"\""> | cr<R"(^"\r\n)">);
  struct STRING : terminal<as<unquote>(lit<"\""> + stringBody + lit<"\"">)> {};

  struct NUMBER : terminal<as<double>((some(d) + option(lit<"."> + many(d))) |
                                      (lit<"."> + some(d)))> {};

  // String concatenation: central to the language, and reached from no
  // parser rule in the source expression grammar, so it is added here on
  // the same footing as the arithmetic operators around it.


  struct Term;
  struct Expression;
  struct Variable;
  struct FunctionCall;
  struct Condition;
  struct Arg;
  struct Command;
  struct IfCommand;

  // A subscript list, in the same shape a function call already takes.
  // Without one, no local or global array is spellable at all -- the
  // defining data structure of the language.
  struct Variable
      : rule<create<ast::Variable> +
             many(flag<&ast::Variable::global>(lit<"^">) |
                  flag<&ast::Variable::extrinsic>(lit<"&">)) +
             assign<&ast::Variable::name>(ref<ID>) +
             option(lit<"("> +
                    some(append<&ast::Variable::subscripts>(ref<Arg>),
                         lit<",">) +
                    lit<")">)> {};

  // A function argument admits a string literal exactly where a command's
  // argument list does ($extract(x,"a","b")): both read the same element.
  struct FunctionCall
      : rule<create<ast::FunctionCall> + lit<"$"> +
             assign<&ast::FunctionCall::name>(ref<ID>) +
             option(lit<"("> +
                    some(append<&ast::FunctionCall::args>(ref<Arg>),
                         lit<",">) +
                    lit<")">)> {};

  struct Term : rule<ref<Variable> | ref<FunctionCall> |
                         create<ast::StringArg>(
                             assign<&ast::StringArg::value>(ref<STRING>)) |
                         create<ast::NumberLiteral>(
                             assign<&ast::NumberLiteral::value>(ref<NUMBER>)) |
                         create<ast::GroupedExpression>(
                             lit<"("> +
                             assign<&ast::GroupedExpression::expression>(
                                 ref<Expression>) +
                             lit<")">)> {};

  struct UnaryTerm
      : rule<create<ast::UnaryExpression>(
                 assign<&ast::UnaryExpression::op>(lit<"+"> | lit<"-"> |
                                                   lit<"'"> | lit<"@">) +
                 assign<&ast::UnaryExpression::operand>(ref<UnaryTerm>)) |
             ref<Term>> {};

  // No precedence step: every operator stands on the one level MUMPS itself
  // evaluates, strictly left to right.
  struct Expression
      : infix<UnaryTerm, ast::BinaryExpression,
              left(as<ast::Operator::add>(many(ref<SPACE>) + lit<"+">) |
                   as<ast::Operator::subtract>(many(ref<SPACE>) + lit<"-">) |
                   as<ast::Operator::multiply>(many(ref<SPACE>) + lit<"*">) |
                   as<ast::Operator::divide>(many(ref<SPACE>) + lit<"/">) |
                   as<ast::Operator::intdivide>(many(ref<SPACE>) + lit<"\\">) |
                   as<ast::Operator::modulo>(many(ref<SPACE>) + lit<"#">) |
                   as<ast::Operator::exponent>(many(ref<SPACE>) + lit<"**">) |
                   as<ast::Operator::concat>(many(ref<SPACE>) + lit<"_">) |
                   as<ast::Operator::ngt>(lit<"'>">) |
                   as<ast::Operator::nlt>(lit<"'<">) |
                   as<ast::Operator::neq>(lit<"'=">) |
                   as<ast::Operator::lt>(lit<"<">) |
                   as<ast::Operator::gt>(lit<">">) |
                   as<ast::Operator::eq>(lit<"=">) |
                   as<ast::Operator::contains>(lit<"[">) |
                   as<ast::Operator::notContains>(lit<"'[">) |
                   as<ast::Operator::follows>(lit<"]">) |
                   as<ast::Operator::notFollows>(lit<"']">) |
                   as<ast::Operator::sortsAfter>(lit<"]]">) |
                   as<ast::Operator::notSortsAfter>(lit<"']]">) |
                   as<ast::Operator::and_>(lit<"&">) |
                   as<ast::Operator::nand>(lit<"'&">) |
                   as<ast::Operator::or_>(lit<"!">) |
                   as<ast::Operator::nor>(lit<"'!">))> {};

  struct Condition : rule<ref<Expression>> {};

  struct Arg : rule<ref<Expression> | create<ast::BangArg>(lit<"!">)> {
  };

  struct Assignment
      : rule<create<ast::Assignment> +
             option(option(lit<"(">) +
                    append<&ast::Assignment::targets>(ref<Arg>) +
                    many(many(ref<SPACE>) + lit<","> +
                         append<&ast::Assignment::targets>(ref<Arg>)) +
                    option(lit<")">)) +
             many(ref<SPACE>) + lit<"="> + many(ref<SPACE>) +
             assign<&ast::Assignment::value>(ref<Arg>)> {};

  struct MergePair
      : rule<create<ast::Assignment> +
             append<&ast::Assignment::targets>(ref<Term>) + lit<"="> +
             assign<&ast::Assignment::value>(ref<Term>)> {};

  struct BreakCommand
      : rule<create<ast::BreakCommand> +
             (ref<CommandWord<"BREAK">> | ref<CommandWord<"B">>)+option(
                 lit<":"> + assign<&ast::BreakCommand::postcondition>(
                                ref<Condition>))> {};

  struct DoTarget
      : rule<(option(ref<LABEL>) + lit<"^"> + ref<ID>) | ref<LABEL>> {};
  struct DoCommand
      : rule<create<ast::DoCommand> +
             (ref<CommandWord<"DO">> | ref<CommandWord<"D">>) +
             option(lit<":"> + assign<&ast::DoCommand::postcondition>(
                                     ref<Condition>)) +
             option(ref<SPACE> + !ref<SPACE> +
                    assign<&ast::DoCommand::name>(ref<DoTarget>) +
                    option(lit<"("> +
                           many(append<&ast::DoCommand::params>(ref<Arg>),
                                lit<",">) +
                           lit<")">))> {};

  // FOR parameters are start[:increment[:limit]], with any number of
  // comma-separated ranges. An argumentless FOR keeps an empty parameter list.
  struct ForParameter
      : rule<create<ast::ForParameter>(
            assign<&ast::ForParameter::start>(ref<Expression>) +
            option(lit<":"> +
                   assign<&ast::ForParameter::step>(ref<Expression>) +
                   option(lit<":"> + assign<&ast::ForParameter::end>(
                                         ref<Expression>))))> {};
  struct ForCommand
      : rule<create<ast::ForCommand>(
            (ref<CommandWord<"FOR">> | ref<CommandWord<"F">>) + ref<SPACE> +
            // Commands must not be viable at a control-variable position:
            // otherwise the keyword i steals the variable in FOR i=... .
            ((assign<&ast::ForCommand::variable>(ref<Term>) + lit<"="> +
              some(append<&ast::ForCommand::parameters>(ref<ForParameter>),
                   lit<",">)) |
             some(ref<SPACE>) | &(ref<NEWLINE> | eof)) +
            many(ref<SPACE>) +
            many(append<&ast::ForCommand::commands>(ref<Command>) +
                 many(ref<SPACE>)) +
            option(lit<":"> +
                   assign<&ast::ForCommand::condition>(ref<Condition>)))> {
  };

  struct HaltCommand
      : rule<create<ast::HaltCommand> +
             (ref<CommandWord<"HALT">> | ref<CommandWord<"H">>)+option(
                 lit<":"> + assign<&ast::HaltCommand::postcondition>(
                                ref<Condition>))> {};

  struct HangCommand
      : rule<create<ast::HangCommand> +
             (ref<CommandWord<"HANG">> | ref<CommandWord<"H">>)+option(
                 lit<":"> +
                 assign<&ast::HangCommand::postcondition>(ref<Condition>)) +
             some(ref<SPACE>) +
             assign<&ast::HangCommand::duration>(ref<Expression>)> {};

  struct IfCommand
      : rule<create<ast::IfCommand> + (ref<CommandWord<"IF">> | ref<CommandWord<"I">>) +
             option((ref<SPACE> + ref<SPACE> + many(ref<SPACE>) +
                     option(assign<&ast::IfCommand::command>(ref<Command>))) |
                    (ref<SPACE> +
                     assign<&ast::IfCommand::condition>(ref<Condition>) +
                     many(lit<","> + append<&ast::IfCommand::moreConditions>(
                                         ref<Condition>)) +
                     option(some(ref<SPACE>) +
                            assign<&ast::IfCommand::command>(ref<Command>))))> {};

  struct KillCommand
      : rule<create<ast::KillCommand> + (ref<CommandWord<"KILL">> | ref<CommandWord<"K">>) +
             option(lit<":"> + assign<&ast::KillCommand::postcondition>(
                                     ref<Condition>)) +
             option(ref<SPACE> + !ref<SPACE> +
                    ((flag<&ast::KillCommand::exclusive>(lit<"(">) +
                      some(append<&ast::KillCommand::args>(ref<Variable>),
                           lit<",">) + lit<")">) |
                     some(append<&ast::KillCommand::args>(ref<Arg>),
                          many(ref<SPACE>) + lit<",">)))> {};

  struct MergeCommand
      : rule<create<ast::MergeCommand> +
             (ref<CommandWord<"MERGE">> | ref<CommandWord<"M">>)+option(
                 lit<":"> + assign<&ast::MergeCommand::postcondition>(
                                ref<Condition>)) +
             some(ref<SPACE>) +
             append<&ast::MergeCommand::pairs>(ref<MergePair>) +
             many(lit<","> +
                  append<&ast::MergeCommand::pairs>(ref<MergePair>))> {};

  struct NewCommand
      : rule<create<ast::NewCommand> + (ref<CommandWord<"NEW">> | ref<CommandWord<"N">>) +
             option(lit<":"> + assign<&ast::NewCommand::postcondition>(
                                     ref<Condition>)) +
             option(ref<SPACE> + !ref<SPACE> +
                    ((flag<&ast::NewCommand::exclusive>(lit<"(">) +
                      some(append<&ast::NewCommand::args>(ref<Variable>),
                           lit<",">) + lit<")">) |
                     some(append<&ast::NewCommand::args>(ref<Arg>),
                          many(ref<SPACE>) + lit<",">)))> {};

  struct QuitCommand
      : rule<create<ast::QuitCommand> +
             (ref<CommandWord<"QUIT">> | ref<CommandWord<"Q">>)+option(
                 lit<":"> +
                 assign<&ast::QuitCommand::postcondition>(ref<Condition>)) +
             option(some(ref<SPACE>) +
                    assign<&ast::QuitCommand::value>(ref<Expression>))> {};

  struct ReadCommand
      : rule<create<ast::ReadCommand> +
             (ref<CommandWord<"READ">> | ref<CommandWord<"R">>)+option(
                 lit<":"> +
                 assign<&ast::ReadCommand::postcondition>(ref<Condition>)) +
             some(ref<SPACE>) + append<&ast::ReadCommand::args>(ref<Arg>) +
             many(many(ref<SPACE>) + lit<","> +
                  append<&ast::ReadCommand::args>(ref<Arg>))> {};

  struct SetCommand
      : rule<create<ast::SetCommand> +
             (ref<CommandWord<"SET">> | ref<CommandWord<"S">>)+option(
                 lit<":"> +
                 assign<&ast::SetCommand::postcondition>(ref<Condition>)) +
             some(ref<SPACE>) +
             append<&ast::SetCommand::assigns>(ref<Assignment>) +
             many(lit<","> +
                  append<&ast::SetCommand::assigns>(ref<Assignment>))> {};

  struct ViewCommand
      : rule<create<ast::ViewCommand> +
             (ref<CommandWord<"VIEW">> | ref<CommandWord<"V">>)+option(
                 lit<":"> +
                 assign<&ast::ViewCommand::postcondition>(ref<Condition>)) +
             some(ref<SPACE>) + assign<&ast::ViewCommand::name>(ref<ID>)> {};

  struct WriteCommand
      : rule<create<ast::WriteCommand> +
             (ref<CommandWord<"WRITE">> | ref<CommandWord<"W">>)+option(
                 lit<":"> + assign<&ast::WriteCommand::postcondition>(
                                ref<Condition>)) +
             some(ref<SPACE>) + append<&ast::WriteCommand::args>(ref<Arg>) +
             many(many(ref<SPACE>) + lit<","> +
                  append<&ast::WriteCommand::args>(ref<Arg>))> {};

  struct XecuteCommand
      : rule<create<ast::XecuteCommand> +
             (ref<CommandWord<"XECUTE">> | ref<CommandWord<"X">>)+option(
                 lit<":"> + assign<&ast::XecuteCommand::postcondition>(
                                ref<Condition>)) +
             some(ref<SPACE>) +
             assign<&ast::XecuteCommand::code>(ref<STRING>)> {};

  struct COMMAND_ARGUMENTS
      : terminal<some((lit<"\""> + stringBody + lit<"\"">) |
                      cr<R"(^ \r\n")">)> {};

  static constexpr auto noArgumentCommand = (ref<CommandWord<"ELSE">> | ref<CommandWord<"E">>) |
                                            (ref<CommandWord<"TCOMMIT">> | ref<CommandWord<"TC">>) |
                                            (ref<CommandWord<"TRESTART">> | ref<CommandWord<"TRE">>);
  static constexpr auto argumentCommand =
      (ref<CommandWord<"CLOSE">> | ref<CommandWord<"C">>) | (ref<CommandWord<"GOTO">> | ref<CommandWord<"G">>) |
      (ref<CommandWord<"JOB">> | ref<CommandWord<"J">>) | (ref<CommandWord<"LOCK">> | ref<CommandWord<"L">>) |
      (ref<CommandWord<"OPEN">> | ref<CommandWord<"O">>) | (ref<CommandWord<"TROLLBACK">> | ref<CommandWord<"TRO">>) |
      (ref<CommandWord<"TSTART">> | ref<CommandWord<"TS">>) | (ref<CommandWord<"USE">> | ref<CommandWord<"U">>);
  static constexpr auto otherPostcondition = option(
      lit<":"> + assign<&ast::OtherCommand::postcondition>(ref<Condition>));
  struct OtherCommand
      : rule<create<ast::OtherCommand>(
            (assign<&ast::OtherCommand::keyword>(noArgumentCommand) +
             otherPostcondition) |
            (assign<&ast::OtherCommand::keyword>(argumentCommand) +
             otherPostcondition +
             option(some(ref<SPACE>) + assign<&ast::OtherCommand::argumentText>(
                                           ref<COMMAND_ARGUMENTS>))))> {};

  struct Command
      : rule<ref<SetCommand> | ref<ForCommand> | ref<WriteCommand> |
             ref<ReadCommand> | ref<QuitCommand> | ref<HaltCommand> |
             ref<HangCommand> | ref<NewCommand> | ref<BreakCommand> |
             ref<DoCommand> | ref<KillCommand> | ref<ViewCommand> |
             ref<MergeCommand> | ref<XecuteCommand> | ref<IfCommand> | ref<OtherCommand>> {
  };

  struct Subproc
      : rule<create<ast::Subproc> + assign<&ast::Subproc::name>(ref<ID>) +
             option(lit<"("> +
                    many(append<&ast::Subproc::params>(ref<Variable>),
                                lit<",">) +
                    lit<")">) +
             some(many(ref<SPACE>) +
                  append<&ast::Subproc::commands>(ref<Command>))> {};

  static constexpr auto countLevels = [](std::string_view text) {
    std::uint64_t count = 0;
    for (const auto c : text) if (c == '.') ++count;
    return count;
  };
  struct LEVEL
      : terminal<as<countLevels>(lit<"."> + many(cr<". ">))> {};

  struct CodeLine
      : rule<create<ast::CodeLine> +
             ((assign<&ast::CodeLine::label>(ref<LABEL>) +
               option(lit<"("> +
                      many(append<&ast::CodeLine::params>(ref<Variable>),
                           lit<",">) + lit<")">) + some(ref<SPACE>)) |
              some(ref<SPACE>)) +
             option(assign<&ast::CodeLine::level>(ref<LEVEL>)) +
             option(some(append<&ast::CodeLine::commands>(ref<Command>),
                         some(ref<SPACE>)) |
                    append<&ast::CodeLine::commands>(ref<Subproc>)) +
             many(ref<SPACE>) + ref<NEWLINE>> {};

  struct RoutineDecl
      : rule<create<ast::RoutineDecl> +
             (flag<&ast::RoutineDecl::percent>(
                  assign<&ast::RoutineDecl::name>(ref<PERCENT_ROUTINE_NAME>)) |
              assign<&ast::RoutineDecl::name>(ref<ROUTINE_NAME>)) +
             option(lit<"("> +
                    many(append<&ast::RoutineDecl::params>(ref<Variable>),
                                lit<",">) +
                    lit<")">) +
             many(ref<SPACE>) + ref<NEWLINE>> {};

  struct Document
      : rule<create<ast::Document> +
             some(append<&ast::Document::lines>(ref<CodeLine>) |
                  append<&ast::Document::lines>(ref<RoutineDecl>) |
                  tight(ref<NEWLINE>)) +
             many(ref<SPACE>) + option(ref<NEWLINE>)> {};

  using entry = Document;
  using ignored = TAB;
  using hidden = list<COMMENT>;
};

}  // namespace mumps
