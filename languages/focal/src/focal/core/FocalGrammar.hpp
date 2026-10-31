// The focal language: its grammar.
#pragma once

#include <cmath>
#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "focal/core/ast.hpp"

namespace focal {

inline constexpr auto unquote = [](std::string_view t) {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (t.empty() || t.front() != '"')
    return Result::err("Incomplete FOCAL text literal.");
  t.remove_prefix(1);
  if (!t.empty() && t.back() == '"') t.remove_suffix(1);
  return Result::ok(t);
};

inline constexpr auto run_length = [](std::string_view t) {
  return static_cast<std::int64_t>(t.size());
};

inline constexpr auto bang_run = [](std::string_view t) {
  return ast::BangRun{run_length(t)};
};

inline constexpr auto hash_run = [](std::string_view t) {
  return ast::HashRun{run_length(t)};
};

inline constexpr auto plusmin_op = [](std::string_view t) {
  return t == "+" ? ast::Operator::add : ast::Operator::subtract;
};

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
  if (i < t.size() && (t[i] == 'e' || t[i] == 'E')) {
    ++i;
    const double exp_sign = read_sign();
    value *= std::pow(10.0, exp_sign * read_digits());
  }
  return value;
};

struct Focal : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct EOL : terminal<eol> {};

  static constexpr auto sign = option(cr<"+-">);
  static constexpr auto digits = some(d);
  static constexpr auto mantissa =
      (digits + option(lit<"."> + option(sign + digits))) |
      (lit<"."> + sign + digits);
  struct NUMBER : terminal<mantissa + option(cr<"eE"> + sign + digits)> {};

  struct PLUSMIN : terminal<as<plusmin_op>(cr<"+-">)> {};
  // The sign belongs to the parser: a signed token would turn TYPE1+2
  // into two adjacent numbers before the expression rule can see '+'.
  struct NumberValue
      : rule<as<number_value>(option(ref<PLUSMIN>) + ref<NUMBER>)> {};
  struct AddOperator
      : rule<as<plusmin_op>(ref<PLUSMIN> + !ref<PLUSMIN>)> {};
  struct POWER : terminal<as<ast::Operator::power>(lit<"^">)> {};
  struct MULTIPLY : terminal<as<ast::Operator::multiply>(lit<"*">)> {};
  struct DIVIDE : terminal<as<ast::Operator::divide>(lit<"/">)> {};

  struct BANG : terminal<as<bang_run>(some(lit<"!">))> {};
  struct HASH : terminal<as<hash_run>(some(lit<"#">))> {};

  // Declared before VARIABLE so a builtin name wins the lexical tie at
  // `name(expr)`.
  struct BUILTIN
      : terminal<ikw<"fsin"> | ikw<"fcos"> | ikw<"fexp"> | ikw<"flog"> |
                 ikw<"fatn"> | ikw<"fsqt"> | ikw<"fabs"> | ikw<"fsgn"> |
                 ikw<"fitr">> {};
  struct RANDOM_BUILTIN : terminal<ikw<"fran">> {};

  struct VARIABLE : terminal<cr<"A-Za-z"> + many(cr<"A-Za-z0-9">)> {};

  struct INTEGER : terminal<as<std::int64_t>(some(d))> {};

  struct STRING_LITERAL : terminal<as<unquote>(
      lit<"\""> + many(cr<R"(^"\r\n)">) + (lit<"\""> | &eol | eof))> {
  };

  struct COMMENT : terminal<lit<"C"> + many(cr<R"(^\r\n)">)> {};

  struct Term;
  struct Expression
      : infix<Term, ast::BinaryExpression,
              // DEC FOCAL 1969 gives multiplication priority over division:
              // X/2*Y means X/(2*Y), manual section 2.4.
              levels(left(ref<POWER>), left(ref<MULTIPLY>),
                     left(ref<DIVIDE>), left(ref<AddOperator>))> {};

  struct Term
      : rule<create<ast::GroupedExpression>(
                 lit<"("> +
                 assign<&ast::GroupedExpression::expression>(
                     ref<Expression>) +
                 lit<")">) |
             create<ast::GroupedExpression>(
                 lit<"["> +
                 assign<&ast::GroupedExpression::expression>(
                     ref<Expression>) +
                 lit<"]">) |
             create<ast::GroupedExpression>(
                 lit<"<"> +
                 assign<&ast::GroupedExpression::expression>(
                     ref<Expression>) +
                 lit<">">) |
             create<ast::NumberLiteral>(
                 assign<&ast::NumberLiteral::value>(ref<NumberValue>)) |
             create<ast::BuiltinCall>(
                 assign<&ast::BuiltinCall::name>(ref<BUILTIN>) + lit<"("> +
                 assign<&ast::BuiltinCall::arg>(ref<Expression>) +
                 lit<")">) |
             create<ast::BuiltinCall>(
                 assign<&ast::BuiltinCall::name>(ref<RANDOM_BUILTIN>) + lit<"("> +
                 option(assign<&ast::BuiltinCall::arg>(ref<Expression>)) + lit<")">) |
             create<ast::IndexedVariable>(
                 assign<&ast::IndexedVariable::name>(ref<VARIABLE>) +
                 lit<"("> +
                 assign<&ast::IndexedVariable::index>(ref<Expression>) +
                 lit<")">) |
             create<ast::VariableRef>(
                 assign<&ast::VariableRef::name>(ref<VARIABLE>))> {};

  struct LineNumber
      : rule<create<ast::LineNumber> +
             assign<&ast::LineNumber::group>(ref<INTEGER>) + lit<"."> +
             assign<&ast::LineNumber::line>(ref<INTEGER>)> {};

  struct Command;

  struct AskPair
      : rule<create<ast::AskPair> +
             option(assign<&ast::AskPair::prompt>(ref<STRING_LITERAL>) + lit<",">) +
             some(append<&ast::AskPair::variables>(ref<VARIABLE>), lit<",">)> {
  };

  struct Ask
      : rule<create<ast::AskCommand> + (kw<"ASK"> | kw<"A">) +
             some(append<&ast::AskCommand::pairs>(ref<AskPair>),
                  lit<",">) +
             option(lit<";"> + assign<&ast::AskCommand::next>(ref<Command>))> {};

  struct Do
      : rule<create<ast::DoCommand> + (kw<"DO"> | kw<"D">) +
             option(flag<&ast::DoCommand::all>(ikw<"all">) |
              assign<&ast::DoCommand::target>(ref<LineNumber>) |
              assign<&ast::DoCommand::group>(ref<INTEGER>)) +
             option(lit<";"> + assign<&ast::DoCommand::next>(ref<Command>))> {};

  struct For
      : rule<create<ast::ForCommand> + (kw<"FOR"> | kw<"F">) +
             assign<&ast::ForCommand::variable>(ref<VARIABLE>) + lit<"="> +
             assign<&ast::ForCommand::from>(ref<Expression>) +
             option(lit<","> +
                    ((assign<&ast::ForCommand::step>(ref<Expression>) +
                      lit<","> + assign<&ast::ForCommand::to>(ref<Expression>)) |
                     assign<&ast::ForCommand::to>(ref<Expression>))) +
             lit<";"> +
             assign<&ast::ForCommand::body>(ref<Command>)> {};

  struct Quit : rule<create<ast::QuitCommand> + (kw<"QUIT"> | kw<"Q">) +
                    option(lit<";"> + assign<&ast::QuitCommand::next>(ref<Command>))> {};

  struct Set
      : rule<create<ast::SetCommand> + (kw<"SET"> | kw<"S">) +
             assign<&ast::SetCommand::variable>(ref<VARIABLE>) +
             option(lit<"("> + assign<&ast::SetCommand::index>(ref<Expression>) + lit<")">) + lit<"="> +
             assign<&ast::SetCommand::value>(ref<Expression>) +
             option(lit<";"> +
                    assign<&ast::SetCommand::next>(ref<Command>))> {};

  struct Goto
      : rule<create<ast::GotoCommand> + (kw<"GOTO"> | kw<"G"> | kw<"GO">) +
             option(assign<&ast::GotoCommand::target>(ref<LineNumber>)) +
             option(lit<";"> + assign<&ast::GotoCommand::next>(ref<Command>))> {
  };

  struct If
      : rule<create<ast::IfCommand> + (kw<"IF"> | kw<"I">) +
             assign<&ast::IfCommand::condition>(ref<Expression>) +
             assign<&ast::IfCommand::negative>(ref<LineNumber>) +
             option(lit<","> + assign<&ast::IfCommand::zero>(ref<LineNumber>) +
                    option(lit<","> +
                           assign<&ast::IfCommand::positive>(ref<LineNumber>))) +
             option(lit<";"> +
                    assign<&ast::IfCommand::next>(ref<Command>))> {};

  // Literal/terminal guards inspect adjacent bytes. A rule predicate would
  // see the whole NUMBER token (5.2 or .2), and allow the format to close early.
  struct FORMAT_WIDTH
      : terminal<as<std::int64_t>(some(d) + !cr<"0-9.">)> {};
  struct FormatSpec
      : rule<create<ast::FormatSpec>(
            (lit<"%"> + flag<&ast::FormatSpec::hasWidth>(
                             assign<&ast::FormatSpec::width>(ref<INTEGER>)) +
             flag<&ast::FormatSpec::hasDecimals>(lit<".">) +
             assign<&ast::FormatSpec::decimals>(ref<INTEGER>)) |
            (lit<"%"> + flag<&ast::FormatSpec::hasWidth>(
                             assign<&ast::FormatSpec::width>(ref<FORMAT_WIDTH>))) |
            lit<"%", !d>)> {};

  struct DumpVariables : rule<create<ast::DumpVariables>(lit<"$">)> {};

  static constexpr auto type_item = ref<Expression> | ref<BANG> |
                                    ref<HASH> | ref<STRING_LITERAL> |
                                    ref<FormatSpec> | ref<DumpVariables>;

  struct Type
      : rule<create<ast::TypeCommand> + (kw<"TYPE"> | kw<"T">) +
             append<&ast::TypeCommand::items>(type_item) +
             many(option(lit<",">) + append<&ast::TypeCommand::items>(type_item)) +
             option(lit<";"> +
                    assign<&ast::TypeCommand::next>(ref<Command>))> {};

  struct Return : rule<create<ast::ReturnCommand> + (kw<"RETURN"> | kw<"R">) +
                      option(lit<";"> + assign<&ast::ReturnCommand::next>(ref<Command>))> {};

  struct Write
      : rule<create<ast::WriteCommand> + (kw<"WRITE"> | kw<"W">) +
             option(flag<&ast::WriteCommand::all>(ikw<"all">) |
                    assign<&ast::WriteCommand::target>(ref<LineNumber>) |
                    assign<&ast::WriteCommand::group>(ref<INTEGER>)) +
             option(lit<";"> + assign<&ast::WriteCommand::next>(ref<Command>))> {};

  struct Erase
      : rule<create<ast::EraseCommand>(
            (kw<"ERASE"> | kw<"E">) +
            option(flag<&ast::EraseCommand::all>(kw<"ALL">) |
                   assign<&ast::EraseCommand::target>(ref<LineNumber>) |
                   assign<&ast::EraseCommand::group>(ref<INTEGER>)) +
            option(lit<";"> + assign<&ast::EraseCommand::next>(ref<Command>)))> {};

  struct Modify
      : rule<create<ast::ModifyCommand>(
            (kw<"MODIFY"> | kw<"M">) +
            assign<&ast::ModifyCommand::target>(ref<LineNumber>))> {};

  struct Comment
      : rule<create<ast::CommentCommand> +
             assign<&ast::CommentCommand::text>(ref<COMMENT>)> {};

  struct Command
      : rule<ref<Ask> | ref<Do> | ref<For> | ref<Set> |
             ref<Quit> | ref<Goto> | ref<If> | ref<Type> |
             ref<Return> | ref<Write> | ref<Erase> | ref<Modify> | ref<Comment>> {};

  struct Statement
      : rule<create<ast::Statement> +
             option(assign<&ast::Statement::number>(ref<LineNumber>)) +
             assign<&ast::Statement::command>(ref<Command>)> {};

  struct Program
      : rule<create<ast::Program> +
             many(ref<EOL>) +
             some(append<&ast::Program::statements>(ref<Statement>), some(ref<EOL>)) +
             many(ref<EOL>)> {};

  using entry = Program;
  using ignored = WS;
};

}  // namespace focal
