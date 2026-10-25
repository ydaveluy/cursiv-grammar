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
using Unquoted = cursiv::text_conv<unquote>;

struct Mumps : cursiv::GrammarSpec {
  struct TAB : terminal<lit<"\t">> {};
  struct SPACE : terminal<lit<" ">> {};
  struct NEWLINE : terminal<some(cr<R"(\r\n)">)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9">)> {};

  static constexpr auto stringBody = many(lit<"\"\""> | cr<R"(^")">);
  struct STRING : terminal<lit<"\""> + stringBody + lit<"\"">, Unquoted> {};

  struct NUMBER
      : terminal<(some(d) + option(lit<"."> + many(d))) | (lit<"."> + some(d)),
                 double> {};

  struct AddOp
      : rule<many(ref<SPACE>) + lit<"+">, value<ast::Operator::add>> {};
  struct SubtractOp
      : rule<many(ref<SPACE>) + lit<"-">, value<ast::Operator::subtract>> {};
  struct MultiplyOp
      : rule<many(ref<SPACE>) + lit<"*">, value<ast::Operator::multiply>> {};
  struct DivideOp
      : rule<many(ref<SPACE>) + lit<"/">, value<ast::Operator::divide>> {};
  struct IntDivideOp
      : rule<many(ref<SPACE>) + lit<"\\">,
             value<ast::Operator::intdivide>> {};
  struct ModuloOp
      : rule<many(ref<SPACE>) + lit<"#">, value<ast::Operator::modulo>> {};
  struct ExponentOp
      : rule<many(ref<SPACE>) + lit<"**">, value<ast::Operator::exponent>> {};
  // String concatenation: central to the language, and reached from no
  // parser rule in the source expression grammar, so it is added here on
  // the same footing as the arithmetic operators around it.
  struct ConcatOp
      : rule<many(ref<SPACE>) + lit<"_">, value<ast::Operator::concat>> {};

  struct GtOp : rule<lit<">">, value<ast::Operator::gt>> {};
  struct LtOp : rule<lit<"<">, value<ast::Operator::lt>> {};
  struct NgtOp : rule<lit<"'>">, value<ast::Operator::ngt>> {};
  struct NltOp : rule<lit<"'<">, value<ast::Operator::nlt>> {};
  struct EqOp : rule<lit<"=">, value<ast::Operator::eq>> {};

  struct TermRule;
  struct ExpressionRule;
  struct VariableRule;
  struct FunctionCallRule;
  struct ConditionRule;
  struct ArgRule;
  struct CommandRule;

  // A subscript list, in the same shape a function call already takes.
  // Without one, no local or global array is spellable at all -- the
  // defining data structure of the language.
  struct VariableRule
      : rule<create<ast::Variable> +
             many(flag<&ast::Variable::global>(lit<"^">) |
                  flag<&ast::Variable::extrinsic>(lit<"&">)) +
             assign<&ast::Variable::name>(ref<ID>) +
             option(lit<"("> +
                    some(append<&ast::Variable::subscripts>(ref<ArgRule>),
                         lit<",">) +
                    lit<")">)> {};

  // A function argument admits a string literal exactly where a command's
  // argument list does ($extract(x,"a","b")): both read the same element.
  struct FunctionCallRule
      : rule<create<ast::FunctionCall> + lit<"$"> +
             assign<&ast::FunctionCall::name>(ref<ID>) +
             option(lit<"("> +
                    some(append<&ast::FunctionCall::args>(ref<ArgRule>),
                         lit<",">) +
                    lit<")">)> {};

  struct TermRule : rule<ref<VariableRule> | ref<FunctionCallRule> |
                         create<ast::StringArg>(
                             assign<&ast::StringArg::value>(ref<STRING>)) |
                         create<ast::NumberLiteral>(
                             assign<&ast::NumberLiteral::value>(ref<NUMBER>)) |
                         create<ast::GroupedExpression>(
                             lit<"("> +
                             assign<&ast::GroupedExpression::expression>(
                                 ref<ExpressionRule>) +
                             lit<")">)> {};

  struct UnaryTermRule
      : rule<create<ast::UnaryExpression>(
                 assign<&ast::UnaryExpression::op>(lit<"+"> | lit<"-"> |
                                                   lit<"'"> | lit<"@">) +
                 assign<&ast::UnaryExpression::operand>(ref<UnaryTermRule>)) |
             ref<TermRule>> {};

  // No precedence step: every operator stands on the one level MUMPS itself
  // evaluates, strictly left to right.
  struct ExpressionRule
      : infix<UnaryTermRule, &ast::BinaryExpression::left,
              &ast::BinaryExpression::op, &ast::BinaryExpression::right,
              left(ref<AddOp> | ref<SubtractOp> | ref<MultiplyOp> |
                   ref<DivideOp> | ref<IntDivideOp> | ref<ModuloOp> |
                   ref<ExponentOp> | ref<ConcatOp> | ref<NgtOp> | ref<NltOp> |
                   ref<LtOp> | ref<GtOp> | ref<EqOp>)> {};

  struct ConditionRule : rule<ref<ExpressionRule>> {};

  struct ArgRule : rule<ref<ExpressionRule> | create<ast::BangArg>(lit<"!">)> {
  };

  struct AssignmentRule
      : rule<create<ast::Assignment> +
             option(option(lit<"(">) +
                    append<&ast::Assignment::targets>(ref<ArgRule>) +
                    many(many(ref<SPACE>) + lit<","> +
                         append<&ast::Assignment::targets>(ref<ArgRule>)) +
                    option(lit<")">)) +
             many(ref<SPACE>) + lit<"="> + many(ref<SPACE>) +
             assign<&ast::Assignment::value>(ref<ArgRule>)> {};

  struct MergePairRule
      : rule<create<ast::Assignment> +
             append<&ast::Assignment::targets>(ref<TermRule>) + lit<"="> +
             assign<&ast::Assignment::value>(ref<TermRule>)> {};

  struct BreakCommand
      : rule<create<ast::BreakCommand> +
             (ikw<"BREAK"> | ikw<"B">)+option(
                 lit<":"> + assign<&ast::BreakCommand::postcondition>(
                                ref<ConditionRule>))> {};

  struct DoCommand
      : rule<create<ast::DoCommand> +
             (ikw<"DO"> | ikw<"D">)+option(
                 lit<":"> +
                 assign<&ast::DoCommand::postcondition>(ref<ConditionRule>)) +
             some(ref<SPACE>) + assign<&ast::DoCommand::name>(ref<ID>) +
             option(
                 lit<"("> +
                 option(some(append<&ast::DoCommand::params>(ref<VariableRule>),
                             lit<",">)) +
                 lit<")">)> {};

  // FOR parameters are start[:increment[:limit]], with any number of
  // comma-separated ranges. An argumentless FOR keeps an empty parameter list.
  struct ForParameterRule
      : rule<create<ast::ForParameter>(
            assign<&ast::ForParameter::start>(ref<ExpressionRule>) +
            option(lit<":"> +
                   assign<&ast::ForParameter::step>(ref<ExpressionRule>) +
                   option(lit<":"> + assign<&ast::ForParameter::end>(
                                         ref<ExpressionRule>))))> {};
  struct ForCommand
      : rule<create<ast::ForCommand>(
            (ikw<"FOR"> | ikw<"F">)+some(ref<SPACE>) +
            option(
                assign<&ast::ForCommand::variable>(ref<TermRule>) + lit<"="> +
                some(
                    append<&ast::ForCommand::parameters>(ref<ForParameterRule>),
                    lit<",">)) +
            many(ref<SPACE>) +
            many(append<&ast::ForCommand::commands>(ref<CommandRule>) +
                 many(ref<SPACE>)) +
            option(lit<":"> +
                   assign<&ast::ForCommand::condition>(ref<ConditionRule>)))> {
  };

  struct HaltCommand
      : rule<create<ast::HaltCommand> +
             (ikw<"HALT"> | ikw<"H">)+option(
                 lit<":"> + assign<&ast::HaltCommand::postcondition>(
                                ref<ConditionRule>))> {};

  struct HangCommand
      : rule<create<ast::HangCommand> +
             (ikw<"HANG"> | ikw<"H">)+option(
                 lit<":"> +
                 assign<&ast::HangCommand::postcondition>(ref<ConditionRule>)) +
             some(ref<SPACE>) +
             assign<&ast::HangCommand::duration>(ref<TermRule>)> {};

  struct IfCommand
      : rule<create<ast::IfCommand> + (ikw<"IF"> | ikw<"I">)+some(ref<SPACE>) +
             assign<&ast::IfCommand::condition>(ref<ConditionRule>) +
             many(ref<SPACE>) +
             assign<&ast::IfCommand::command>(ref<CommandRule>)> {};

  struct KillCommand
      : rule<create<ast::KillCommand> +
             (ikw<"KILL"> | ikw<"K">)+option(
                 lit<":"> +
                 assign<&ast::KillCommand::postcondition>(ref<ConditionRule>)) +
             some(ref<SPACE>) + append<&ast::KillCommand::args>(ref<ArgRule>) +
             many(many(ref<SPACE>) + lit<","> +
                  append<&ast::KillCommand::args>(ref<ArgRule>))> {};

  struct MergeCommand
      : rule<create<ast::MergeCommand> +
             (ikw<"MERGE"> | ikw<"M">)+option(
                 lit<":"> + assign<&ast::MergeCommand::postcondition>(
                                ref<ConditionRule>)) +
             some(ref<SPACE>) +
             append<&ast::MergeCommand::pairs>(ref<MergePairRule>) +
             many(lit<","> +
                  append<&ast::MergeCommand::pairs>(ref<MergePairRule>))> {};

  struct NewCommand
      : rule<create<ast::NewCommand> +
             (ikw<"NEW"> | ikw<"N">)+option(
                 lit<":"> +
                 assign<&ast::NewCommand::postcondition>(ref<ConditionRule>)) +
             some(ref<SPACE>) + append<&ast::NewCommand::args>(ref<ArgRule>) +
             many(many(ref<SPACE>) + lit<","> +
                  append<&ast::NewCommand::args>(ref<ArgRule>))> {};

  struct QuitCommand
      : rule<create<ast::QuitCommand> +
             (ikw<"QUIT"> | ikw<"Q">)+option(
                 lit<":"> +
                 assign<&ast::QuitCommand::postcondition>(ref<ConditionRule>)) +
             option(some(ref<SPACE>) +
                    assign<&ast::QuitCommand::value>(ref<TermRule>))> {};

  struct ReadCommand
      : rule<create<ast::ReadCommand> +
             (ikw<"READ"> | ikw<"R">)+option(
                 lit<":"> +
                 assign<&ast::ReadCommand::postcondition>(ref<ConditionRule>)) +
             some(ref<SPACE>) + append<&ast::ReadCommand::args>(ref<ArgRule>) +
             many(many(ref<SPACE>) + lit<","> +
                  append<&ast::ReadCommand::args>(ref<ArgRule>))> {};

  struct SetCommand
      : rule<create<ast::SetCommand> +
             (ikw<"SET"> | ikw<"S">)+option(
                 lit<":"> +
                 assign<&ast::SetCommand::postcondition>(ref<ConditionRule>)) +
             some(ref<SPACE>) +
             append<&ast::SetCommand::assigns>(ref<AssignmentRule>) +
             many(lit<","> +
                  append<&ast::SetCommand::assigns>(ref<AssignmentRule>))> {};

  struct ViewCommand
      : rule<create<ast::ViewCommand> +
             (ikw<"VIEW"> | ikw<"V">)+option(
                 lit<":"> +
                 assign<&ast::ViewCommand::postcondition>(ref<ConditionRule>)) +
             some(ref<SPACE>) + assign<&ast::ViewCommand::name>(ref<ID>)> {};

  struct WriteCommand
      : rule<create<ast::WriteCommand> +
             (ikw<"WRITE"> | ikw<"W">)+option(
                 lit<":"> + assign<&ast::WriteCommand::postcondition>(
                                ref<ConditionRule>)) +
             some(ref<SPACE>) + append<&ast::WriteCommand::args>(ref<ArgRule>) +
             many(many(ref<SPACE>) + lit<","> +
                  append<&ast::WriteCommand::args>(ref<ArgRule>))> {};

  struct XecuteCommand
      : rule<create<ast::XecuteCommand> +
             (ikw<"XECUTE"> | ikw<"X">)+option(
                 lit<":"> + assign<&ast::XecuteCommand::postcondition>(
                                ref<ConditionRule>)) +
             some(ref<SPACE>) +
             assign<&ast::XecuteCommand::code>(ref<STRING>)> {};

  struct COMMAND_ARGUMENTS
      : terminal<some((lit<"\""> + stringBody + lit<"\"">) |
                      cr<R"(^ \r\n")">)> {};

  static constexpr auto noArgumentCommand = (ikw<"ELSE"> | ikw<"E">) |
                                            (ikw<"TCOMMIT"> | ikw<"TC">) |
                                            (ikw<"TRESTART"> | ikw<"TRE">);
  static constexpr auto argumentCommand =
      (ikw<"CLOSE"> | ikw<"C">) | (ikw<"GOTO"> | ikw<"G">) |
      (ikw<"JOB"> | ikw<"J">) | (ikw<"LOCK"> | ikw<"L">) |
      (ikw<"OPEN"> | ikw<"O">) | (ikw<"TROLLBACK"> | ikw<"TRO">) |
      (ikw<"TSTART"> | ikw<"TS">) | (ikw<"USE"> | ikw<"U">);
  static constexpr auto otherPostcondition = option(
      lit<":"> + assign<&ast::OtherCommand::postcondition>(ref<ConditionRule>));
  struct OtherCommandRule
      : rule<create<ast::OtherCommand>(
            (assign<&ast::OtherCommand::keyword>(noArgumentCommand) +
             otherPostcondition) |
            (assign<&ast::OtherCommand::keyword>(argumentCommand) +
             otherPostcondition +
             option(some(ref<SPACE>) + assign<&ast::OtherCommand::argumentText>(
                                           ref<COMMAND_ARGUMENTS>))))> {};

  struct CommandRule
      : rule<ref<SetCommand> | ref<ForCommand> | ref<WriteCommand> |
             ref<ReadCommand> | ref<QuitCommand> | ref<HaltCommand> |
             ref<HangCommand> | ref<NewCommand> | ref<BreakCommand> |
             ref<DoCommand> | ref<KillCommand> | ref<ViewCommand> |
             ref<MergeCommand> | ref<XecuteCommand> | ref<OtherCommandRule>> {
  };

  struct SubprocRule
      : rule<create<ast::Subproc> + assign<&ast::Subproc::name>(ref<ID>) +
             option(lit<"("> +
                    option(some(append<&ast::Subproc::params>(ref<VariableRule>),
                                lit<",">)) +
                    lit<")">) +
             some(many(ref<SPACE>) +
                  append<&ast::Subproc::commands>(ref<CommandRule>))> {};

  // A nested block's dotted prefix is directly followed by the command in
  // the source rule, with no space admitted between -- so a block body
  // written the way every dotted MUMPS example writes it (a space after
  // the dots, `. WRITE x`) would not parse. Nesting under IF/FOR/DO is
  // otherwise unreachable, so that trailing space is made optional here,
  // the same way the label branch already requires one after the name.
  struct CodeLineRule
      : rule<create<ast::CodeLine> +
             ((assign<&ast::CodeLine::label>(ref<ID>) + some(ref<SPACE>)) |
              (some(ref<SPACE>) + some(lit<".">) + many(ref<SPACE>)) |
              some(ref<SPACE>)) +
             option(some(append<&ast::CodeLine::commands>(ref<CommandRule>),
                         some(ref<SPACE>)) |
                    append<&ast::CodeLine::commands>(ref<IfCommand>) |
                    append<&ast::CodeLine::commands>(ref<SubprocRule>)) +
             many(ref<SPACE>) + ref<NEWLINE>> {};

  struct RoutineDeclRule
      : rule<create<ast::RoutineDecl> +
             option(flag<&ast::RoutineDecl::percent>(lit<"%">)) +
             assign<&ast::RoutineDecl::name>(ref<ID>) +
             option(lit<"("> +
                    option(some(append<&ast::RoutineDecl::params>(ref<VariableRule>),
                                lit<",">)) +
                    lit<")">) +
             many(ref<SPACE>) + ref<NEWLINE>> {};

  struct DocumentRule
      : rule<create<ast::Document> +
             some(append<&ast::Document::lines>(ref<CodeLineRule>) |
                  append<&ast::Document::lines>(ref<RoutineDeclRule>) |
                  tight(ref<NEWLINE>)) +
             many(ref<SPACE>) + option(ref<NEWLINE>)> {};

  using entry = DocumentRule;
  using ignore = TAB;
  using hidden = list<COMMENT>;
};

}  // namespace mumps
