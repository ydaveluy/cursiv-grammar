// The focal language: its grammar.
#pragma once

#include <cmath>
#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "focal/core/ast.hpp"

namespace focal {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

inline constexpr auto run_length = [](std::string_view t) {
  return static_cast<std::int64_t>(t.size());
};

inline constexpr auto bang_run = [](std::string_view t) {
  return ast::BangRun{run_length(t)};
};
using BangConv = cursiv::text_conv<bang_run>;

inline constexpr auto hash_run = [](std::string_view t) {
  return ast::HashRun{run_length(t)};
};
using HashConv = cursiv::text_conv<hash_run>;

inline constexpr auto plusmin_op = [](std::string_view t) {
  return t == "+" ? ast::Operator::add : ast::Operator::subtract;
};
using PlusMinConv = cursiv::text_conv<plusmin_op>;

inline constexpr auto mulop_op = [](std::string_view t) {
  if (t == "*") return ast::Operator::multiply;
  if (t == "/") return ast::Operator::divide;
  return ast::Operator::power;
};
using MulOpConv = cursiv::text_conv<mulop_op>;

// The mantissa's parts read separately (a sign may sit before or after the
// dot), so the value is assembled by hand rather than handed to strtod.
inline constexpr auto number_value = [](std::string_view t) {
  std::size_t i = 0;
  const auto read_sign = [&] {
    double s = 1.0;
    if (i < t.size() && (t[i] == '+' || t[i] == '-')) {
      if (t[i] == '-') s = -1.0;
      ++i;
    }
    return s;
  };
  const auto read_digits = [&] {
    double v = 0.0;
    while (i < t.size() && t[i] >= '0' && t[i] <= '9') {
      v = v * 10.0 + static_cast<double>(t[i] - '0');
      ++i;
    }
    return v;
  };

  const double whole_sign = read_sign();
  double value = whole_sign * read_digits();
  if (i < t.size() && t[i] == '.') {
    ++i;
    const double frac_sign = read_sign();
    double frac = 0.0;
    double div = 1.0;
    while (i < t.size() && t[i] >= '0' && t[i] <= '9') {
      frac = frac * 10.0 + static_cast<double>(t[i] - '0');
      div *= 10.0;
      ++i;
    }
    value += whole_sign * frac_sign * frac / div;
  }
  if (i < t.size() && t[i] == 'e') {
    ++i;
    const double exp_sign = read_sign();
    value *= std::pow(10.0, exp_sign * read_digits());
  }
  return value;
};
using NumberConv = cursiv::text_conv<number_value>;

struct Focal : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};

  static constexpr auto sign = option(cr<"+-">);
  static constexpr auto digits = some(d);
  static constexpr auto mantissa =
      (sign + digits + option(lit<"."> + option(sign + digits))) |
      (lit<"."> + sign + digits);
  struct NUMBER : terminal<mantissa + option(lit<"e"> + sign + digits),
                           NumberConv> {};

  struct PLUSMIN : terminal<cr<"+-">, PlusMinConv> {};
  struct MULOP : terminal<cr<"*/^">, MulOpConv> {};

  struct BANG : terminal<some(lit<"!">), BangConv> {};
  struct HASH : terminal<some(lit<"#">), HashConv> {};

  // Declared before VARIABLE so a builtin name wins the lexical tie at
  // `name(expr)`.
  struct BUILTIN
      : terminal<kw<"fsin"> | kw<"fcos"> | kw<"fexp"> | kw<"flog"> |
                 kw<"fatn"> | kw<"fsqt"> | kw<"fabs"> | kw<"fsgn"> |
                 kw<"fitr"> | kw<"fran">> {};

  struct VARIABLE : terminal<cr<"A-Za-z"> + many(cr<"A-Za-z0-9">)> {};

  struct INTEGER : terminal<some(d), std::int64_t> {};

  struct STRING_LITERAL : terminal<through(lit<"\"">, lit<"\"">), Unquoted> {};

  struct COMMENT : terminal<lit<"COMMENT"> + many(cr<R"(^\r\n)">)> {};

  struct TermRule;
  struct ExpressionRule
      : infix<TermRule, &ast::BinaryExpression::left,
              &ast::BinaryExpression::op, &ast::BinaryExpression::right,
              (left(ref<MULOP>) >> left(ref<PLUSMIN>))> {};

  struct TermRule
      : rule<create<ast::GroupedExpression>(
                 lit<"("> +
                 assign<&ast::GroupedExpression::expression>(
                     ref<ExpressionRule>) +
                 lit<")">) |
             create<ast::GroupedExpression>(
                 lit<"["> +
                 assign<&ast::GroupedExpression::expression>(
                     ref<ExpressionRule>) +
                 lit<"]">) |
             create<ast::GroupedExpression>(
                 lit<"<"> +
                 assign<&ast::GroupedExpression::expression>(
                     ref<ExpressionRule>) +
                 lit<">">) |
             create<ast::NumberLiteral>(
                 assign<&ast::NumberLiteral::value>(ref<NUMBER>)) |
             create<ast::BuiltinCall>(
                 assign<&ast::BuiltinCall::name>(ref<BUILTIN>) + lit<"("> +
                 assign<&ast::BuiltinCall::arg>(ref<ExpressionRule>) +
                 lit<")">) |
             create<ast::IndexedVariable>(
                 assign<&ast::IndexedVariable::name>(ref<VARIABLE>) +
                 lit<"("> +
                 assign<&ast::IndexedVariable::index>(ref<ExpressionRule>) +
                 lit<")">) |
             create<ast::VariableRef>(
                 assign<&ast::VariableRef::name>(ref<VARIABLE>))> {};

  struct LineNumberRule
      : rule<create<ast::LineNumber> +
             assign<&ast::LineNumber::group>(ref<INTEGER>) + lit<"."> +
             assign<&ast::LineNumber::line>(ref<INTEGER>)> {};

  struct CommandRule;

  struct AskPairRule
      : rule<create<ast::AskPair> +
             assign<&ast::AskPair::prompt>(ref<STRING_LITERAL>) + lit<","> +
             some(append<&ast::AskPair::variables>(ref<VARIABLE>), lit<",">)> {
  };

  struct AskRule
      : rule<create<ast::AskCommand> + (kw<"ASK"> | kw<"A">) +
             some(append<&ast::AskCommand::pairs>(ref<AskPairRule>),
                  lit<",">)> {};

  struct DoRule
      : rule<create<ast::DoCommand> + (kw<"DO"> | kw<"D">) +
             (flag<&ast::DoCommand::all>(kw<"all">) |
              assign<&ast::DoCommand::target>(ref<LineNumberRule>) |
              assign<&ast::DoCommand::group>(ref<INTEGER>))> {};

  struct ForRule
      : rule<create<ast::ForCommand> + (kw<"FOR"> | kw<"F">) +
             assign<&ast::ForCommand::variable>(ref<VARIABLE>) + lit<"="> +
             assign<&ast::ForCommand::from>(ref<ExpressionRule>) + lit<","> +
             assign<&ast::ForCommand::to>(ref<ExpressionRule>) +
             option(lit<","> +
                    assign<&ast::ForCommand::step>(ref<ExpressionRule>)) +
             lit<";"> +
             assign<&ast::ForCommand::body>(ref<CommandRule>)> {};

  struct QuitRule : rule<create<ast::QuitCommand> + kw<"QUIT">> {};

  struct SetRule
      : rule<create<ast::SetCommand> + (kw<"SET"> | kw<"S">) +
             assign<&ast::SetCommand::variable>(ref<VARIABLE>) + lit<"="> +
             assign<&ast::SetCommand::value>(ref<ExpressionRule>) +
             option(lit<";"> +
                    assign<&ast::SetCommand::next>(ref<CommandRule>))> {};

  struct GotoRule
      : rule<create<ast::GotoCommand> + (kw<"GOTO"> | kw<"G">) +
             option(assign<&ast::GotoCommand::target>(ref<LineNumberRule>))> {
  };

  struct IfRule
      : rule<create<ast::IfCommand> + kw<"IF"> +
             assign<&ast::IfCommand::condition>(ref<ExpressionRule>) +
             assign<&ast::IfCommand::negative>(ref<LineNumberRule>) +
             lit<","> +
             assign<&ast::IfCommand::zero>(ref<LineNumberRule>) + lit<","> +
             assign<&ast::IfCommand::positive>(ref<LineNumberRule>) +
             option(lit<";"> +
                    assign<&ast::IfCommand::next>(ref<CommandRule>))> {};

  struct FormatSpecRule
      : rule<create<ast::FormatSpec> + lit<"%"> +
             assign<&ast::FormatSpec::width>(ref<INTEGER>) + lit<"."> +
             assign<&ast::FormatSpec::decimals>(ref<INTEGER>)> {};

  static constexpr auto type_item = ref<ExpressionRule> | ref<BANG> |
                                    ref<HASH> | ref<STRING_LITERAL> |
                                    ref<FormatSpecRule>;

  struct TypeRule
      : rule<create<ast::TypeCommand> + (kw<"TYPE"> | kw<"T">) +
             some(append<&ast::TypeCommand::items>(type_item), lit<",">) +
             option(lit<";"> +
                    assign<&ast::TypeCommand::next>(ref<CommandRule>))> {};

  struct ReturnRule : rule<create<ast::ReturnCommand> + kw<"RETURN">> {};

  struct WriteRule
      : rule<create<ast::WriteCommand> + kw<"WRITE"> +
             option(assign<&ast::WriteCommand::target>(ref<LineNumberRule>) |
                    assign<&ast::WriteCommand::group>(ref<INTEGER>))> {};

  struct CommentRule
      : rule<create<ast::CommentCommand> +
             assign<&ast::CommentCommand::text>(ref<COMMENT>)> {};

  struct CommandRule
      : rule<ref<AskRule> | ref<DoRule> | ref<ForRule> | ref<SetRule> |
             ref<QuitRule> | ref<GotoRule> | ref<IfRule> | ref<TypeRule> |
             ref<ReturnRule> | ref<WriteRule> | ref<CommentRule>> {};

  struct StatementRule
      : rule<create<ast::Statement> +
             assign<&ast::Statement::number>(ref<LineNumberRule>) +
             assign<&ast::Statement::command>(ref<CommandRule>)> {};

  struct ProgramRule
      : rule<create<ast::Program> +
             some(append<&ast::Program::statements>(ref<StatementRule>))> {};

  using entry = ProgramRule;
  using ignore = WS;
};

}  // namespace focal
