// The velocity language: its grammar. A document is literal text with two
// kinds of island -- `$...` references and `#...` directives -- read with
// per-state lexing: the state a rule is in already restricts what token
// is legal.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "velocity/core/ast.hpp"

namespace velocity {

inline constexpr auto unquote_string =
    [](std::string_view t, std::pmr::memory_resource* mem) {
      if (t.size() < 2) return t;
      const char q = t.front();
      const std::string_view body = t.substr(1, t.size() - 2);
      if (body.find(q) == std::string_view::npos) return body;
      const std::span<char> room = cursiv::text_room(mem, body.size());
      std::size_t n = 0;
      for (std::size_t i = 0; i < body.size(); ++i) {
        room[n++] = body[i];
        if (body[i] == q && i + 1 < body.size() && body[i + 1] == q) ++i;
      }
      return std::string_view(room.data(), n);
    };

inline constexpr auto unescape_block = [](std::string_view t) {
  return t.size() < 6 ? t : t.substr(3, t.size() - 6);
};

struct Velocity : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_">;
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct LINE_COMMENT : terminal<lit<"##"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"#*">, lit<"*#">)> {};

  // A run of literal text preserves whitespace and stops at `\`
  // (escape), `#` (directive/comment), `$` (reference) and `.[(` (a
  // reference's chain continuation -- excluded so a run can never
  // out-munch a single chain-opener byte and strand it as plain text).
  struct TEXT : terminal<some(cr<R"(^\\#$.[()">)> {};

  struct ESCAPED_CHAR : terminal<lit<"\\"> + any> {};

  struct ESCAPED_BLOCK
      : terminal<as<unescape_block>(through(lit<"#[[">, lit<"]]#">))> {};

  struct DOLLAR : terminal<lit<"$">> {};
  struct DOLLAR_EXCL : terminal<lit<"$"> + many(lit<"\\">) + lit<"!">> {};
  struct DOLLAR_OBRACE : terminal<lit<"${">> {};
  struct DOLLAR_EXCL_OBRACE
      : terminal<lit<"$"> + many(lit<"\\">) + lit<"!{">> {};

  static constexpr auto IdStart = cr<"a-zA-Z_">;
  struct Id : terminal<IdStart + many(cr<"a-zA-Z0-9_">)> {};
  struct LiteralDollar : terminal<
      lit<"$!{", !IdStart> | lit<"${", !IdStart> |
      lit<"$!", !(IdStart | lit<"{">)> | lit<"$", !(IdStart | cr<"{!">)>> {};
  struct LiteralHash : terminal<lit<"#"> + !(IdStart | cr<"{@*#">)> {};

  struct DOUBLE_STRING : terminal<as<unquote_string>(
      lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">)> {};
  struct SINGLE_STRING : terminal<as<unquote_string>(
      lit<"'"> + many(lit<"''"> | cr<R"(^')">) + lit<"'">)> {};


  struct INTEGER : terminal<as<std::int64_t>(some(d))> {};

  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct FLOAT
      : terminal<as<double>((many(d) + lit<"."> + some(d) + option(Exponent)) |
                            (some(d) + lit<".", !lit<".">> + option(Exponent)) |
                            (some(d) + Exponent))> {};

  // One token per operation, value-carrying: the infix table names them by
  // what they mean. A relational/logical operator has both a symbol and a
  // word spelling (`==`/`eq`, `&&`/`and`); either lexes the same value.
  struct GE : rule<as<ast::Operator::ge>(option(ref<WS>) + (lit<">=">))> {};
  struct GT : rule<as<ast::Operator::gt>(option(ref<WS>) + (lit<">">))> {};
  struct AND
      : rule<as<ast::Operator::logical_and>(option(ref<WS>) + (lit<"&&">))> {};

  struct Expression;

  static constexpr auto SP = option(ref<WS>);

  // A property/index/call access, wrapping whatever the chain has built so
  // far as `object` -- a left fold, the way stringtemplate's MemberExpr
  // reads a `.field`/`.( )` chain. `gap::none` before each opener: the
  // reference reads a chain step glued to its base, with no trivia (not
  // even a blank line) between them -- without it, a lone `$x.` at the end
  // of a paragraph reaches across the gap and grabs the next paragraph's
  // first word as a property name.
  static constexpr auto Call =
      nest<ast::CallAccess, &ast::CallAccess::object>(
          gap::none + lit<"("> +
          option(some(append<&ast::CallAccess::args>(ref<Expression>), SP + lit<",">)) +
          SP + lit<")">);
  static constexpr auto ChainStep =
      (nest<ast::PropertyAccess, &ast::PropertyAccess::object>(
          gap::none + lit<"."> + gap::none +
          assign<&ast::PropertyAccess::name>(ref<Id>)) + (Call | !lit<"(">)) |
      nest<ast::IndexAccess, &ast::IndexAccess::object>(
          gap::none + lit<"["> + assign<&ast::IndexAccess::index>(ref<Expression>) +
          SP + lit<"]">);

  // The bare base of a reference, no chain: `#foreach` reads exactly this
  // for its loop variable.
  struct VariableRefBase
      : rule<(flag<&ast::VariableRef::quiet>(ref<DOLLAR_EXCL>) |
              ref<DOLLAR>) +
             assign<&ast::VariableRef::name>(ref<Id>)> {};

  // Keep each reference chain in one rule for the heterogeneous text atoms.
  struct VariableRefChain
      : rule<ref<VariableRefBase> + many(ChainStep) +
             !((lit<"."> + ref<Id>) | lit<"[">)> {};

  struct FormalRefBase
      : rule<(flag<&ast::FormalRef::quiet>(ref<DOLLAR_EXCL_OBRACE>) |
              ref<DOLLAR_OBRACE>) +
             assign<&ast::FormalRef::name>(ref<Id>)> {};

  struct FormalRef : rule<ref<FormalRefBase> + many(ChainStep) +
      option(nest<ast::DefaultExpr, &ast::DefaultExpr::reference>(
          SP + lit<"|"> + assign<&ast::DefaultExpr::fallback>(ref<Expression>))) +
      SP + lit<"}">> {};

  struct IdExpr : rule<SP + assign<&ast::IdExpr::name>(ref<Id>)> {};
  struct BoolLit : rule<SP + (flag<&ast::BoolLit::value>(kw<"true">) | kw<"false">)> {};
  struct StringLit : rule<SP +
      (flag<&ast::StringLit::doubleQuoted>(assign<&ast::StringLit::value>(ref<DOUBLE_STRING>)) |
       assign<&ast::StringLit::value>(ref<SINGLE_STRING>))> {};
  struct IntLit : rule<(SP + assign<&ast::IntLit::value>(ref<INTEGER>))> {};
  struct FloatLit : rule<(SP + assign<&ast::FloatLit::value>(ref<FLOAT>))> {};

  struct RangeBound : rule<(SP + ref<VariableRefChain>) | (SP + ref<FormalRef>) |
      create<ast::UnaryExpr>(SP + assign<&ast::UnaryExpr::op>(as<ast::Operator::negate>(lit<"-">)) +
          assign<&ast::UnaryExpr::operand>(ref<IntLit>)) | ref<IntLit>> {};
  struct RangeValue : rule<create<ast::BinaryExpr>(
      assign<&ast::BinaryExpr::left>(ref<RangeBound>) +
      assign<&ast::BinaryExpr::op>(as<ast::Operator::range>(SP + lit<"..">)) +
      assign<&ast::BinaryExpr::right>(ref<RangeBound>))> {};
  struct ListLit : rule<create<ast::ListLit> + SP + lit<"["> +
      option(append<&ast::ListLit::items>(ref<RangeValue>) |
          some(append<&ast::ListLit::items>(ref<Expression>), SP + lit<",">)) +
      SP + lit<"]">> {};

  struct MapEntry
      : rule<assign<&ast::MapEntry::key>(ref<Expression>) +
             (SP + lit<":">)+assign<&ast::MapEntry::value>(ref<Expression>)> {};

  struct MapLit
      : rule<create<ast::MapLit> +
             (SP + lit<"{">)+option(some(
                 append<&ast::MapLit::entries>(ref<MapEntry>), (SP + lit<",">))) +
             (SP + lit<"}">)> {};

  struct Primary
      : rule<((SP + lit<"(">)+ref<Expression> + (SP + lit<")">)) | ref<ListLit> |
             ref<MapLit> | (SP + ref<VariableRefBase>) | (SP + ref<FormalRef>) |
             ref<StringLit> | ref<IntLit> | ref<FloatLit> | ref<BoolLit>> {};

  // Unary operations bind more tightly than every infix level.
  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
                 (SP + assign<&ast::UnaryExpr::op>(as<ast::Operator::logical_not>(option(ref<WS>) + (lit<"!"> | kw<"not">)) | as<ast::Operator::negate>(option(ref<WS>) + (lit<"-">)))) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>)) |
             (ref<Primary> + some(ChainStep)) | ref<Primary>> {};

  // VTL expression precedence; a range is recognized only inside brackets.
  struct Expression
      : infix<UnaryExpr, ast::BinaryExpr,
              levels(left(as<ast::Operator::mul>(option(ref<WS>) + (lit<"*">)) | as<ast::Operator::div>(option(ref<WS>) + (lit<"/">)) | as<ast::Operator::mod>(option(ref<WS>) + (lit<"%">))),
                     left(as<ast::Operator::add>(option(ref<WS>) + (lit<"+">)) | as<ast::Operator::sub>(option(ref<WS>) + (lit<"-">))),
                     left(as<ast::Operator::le>(option(ref<WS>) + (lit<"<=">)) | ref<GE> | as<ast::Operator::lt>(option(ref<WS>) + (lit<"<">)) | ref<GT> | as<ast::Operator::le>(option(ref<WS>) + (kw<"le">)) |
                          as<ast::Operator::ge>(option(ref<WS>) + (kw<"ge">)) | as<ast::Operator::lt>(option(ref<WS>) + (kw<"lt">)) | as<ast::Operator::gt>(option(ref<WS>) + (kw<"gt">))),
                     left(as<ast::Operator::eq>(option(ref<WS>) + (lit<"==">)) | as<ast::Operator::ne>(option(ref<WS>) + (lit<"!=">)) | as<ast::Operator::eq>(option(ref<WS>) + (kw<"eq">)) | as<ast::Operator::ne>(option(ref<WS>) + (kw<"ne">))),
                     left(ref<AND> | as<ast::Operator::logical_and>(option(ref<WS>) + (kw<"and">))), left(as<ast::Operator::logical_or>(option(ref<WS>) + (lit<"||">)) | as<ast::Operator::logical_or>(option(ref<WS>) + (kw<"or">))))> {};

  struct BlockRule;

  // Every directive keyword also has a brace spelling (`#{stop}`, `#{else}`)
  // used to glue a directive to text that would otherwise fuse with its
  // name; both read the same keyword.
  static constexpr auto KwEnd = kw<"end"> | lit<"{end}">;
  static constexpr auto KwElseif = kw<"elseif"> | lit<"{elseif}">;
  static constexpr auto KwIf = kw<"if"> | lit<"{if}">;
  static constexpr auto KwElse = kw<"else"> | lit<"{else}">;
  static constexpr auto KwForeach = kw<"foreach"> | lit<"{foreach}">;
  static constexpr auto KwBreak = kw<"break"> | lit<"{break}">;
  static constexpr auto KwStop = kw<"stop"> | lit<"{stop}">;
  static constexpr auto KwSet = kw<"set"> | lit<"{set}">;
  static constexpr auto KwMacro = kw<"macro"> | lit<"{macro}">;
  static constexpr auto KwParse = kw<"parse"> | lit<"{parse}">;
  static constexpr auto KwDefine = kw<"define"> | lit<"{define}">;
  static constexpr auto KwInclude = kw<"include"> | lit<"{include}">;
  static constexpr auto KwEvaluate = kw<"evaluate"> | lit<"{evaluate}">;

  static constexpr auto End = lit<"#"> + KwEnd;

  struct IfBranch
      : rule<lit<"#"> + KwElseif +
             (SP + lit<"(">)+assign<&ast::IfBranch::cond>(ref<Expression>) +
             (SP + lit<")">)+assign<&ast::IfBranch::body>(ref<BlockRule>)> {};

  struct IfDirective
      : rule<lit<"#"> + KwIf +
             (SP + lit<"(">)+assign<&ast::IfDirective::cond>(ref<Expression>) +
             (SP + lit<")">)+assign<&ast::IfDirective::thenBlock>(ref<BlockRule>) +
             many(append<&ast::IfDirective::elseifs>(ref<IfBranch>)) +
             option(lit<"#"> + KwElse +
                    assign<&ast::IfDirective::elseBlock>(ref<BlockRule>)) +
             End> {};

  struct ForeachDirective
      : rule<lit<"#"> + KwForeach +
             (SP + lit<"(">) + SP + assign<&ast::ForeachDirective::var>(ref<VariableRefBase>) +
             (SP + kw<"in">)+assign<&ast::ForeachDirective::iterable>(ref<Expression>) +
             (SP + lit<")">)+assign<&ast::ForeachDirective::body>(ref<BlockRule>) +
             End> {};

  template <auto Member>
  static constexpr auto OptionalArgument =
      ((SP + lit<"(">) + option(assign<Member>(ref<Expression>)) + SP + lit<")">) |
      !(SP + lit<"(">);
  struct BreakDirective : rule<create<ast::BreakDirective> + lit<"#"> + KwBreak +
      OptionalArgument<&ast::BreakDirective::argument>> {};
  struct StopDirective : rule<create<ast::StopDirective> + lit<"#"> + KwStop +
      OptionalArgument<&ast::StopDirective::argument>> {};

  struct SetDirective
      : rule<lit<"#"> + KwSet +
             (SP + lit<"(">) + SP + assign<&ast::SetDirective::target>(ref<VariableRefChain> | ref<FormalRef>) +
             (SP + lit<"=">)+assign<&ast::SetDirective::value>(ref<Expression>) +
             (SP + lit<")">)> {};

  struct MacroParameter : rule<(SP + ref<VariableRefChain>) +
      option(nest<ast::DefaultExpr, &ast::DefaultExpr::reference>(
          SP + lit<"="> + assign<&ast::DefaultExpr::fallback>(ref<Expression>)))> {};
  struct MacroDirective : rule<lit<"#"> + KwMacro + SP + lit<"("> +
      append<&ast::MacroDirective::params>(ref<IdExpr>) +
      many(option(SP + lit<",">) + append<&ast::MacroDirective::params>(ref<MacroParameter>)) +
      SP + lit<")"> + assign<&ast::MacroDirective::body>(ref<BlockRule>) + End> {};

  struct ParseDirective
      : rule<lit<"#"> + KwParse +
             (SP + lit<"(">)+assign<&ast::ParseDirective::path>(ref<Expression>) +
             (SP + lit<")">)> {};

  struct DefineDirective
      : rule<lit<"#"> + KwDefine +
             (SP + lit<"(">)+assign<&ast::DefineDirective::target>(ref<Expression>) +
             (SP + lit<")">)+assign<&ast::DefineDirective::body>(ref<BlockRule>) +
             End> {};

  struct IncludeDirective
      : rule<lit<"#"> + KwInclude +
             (SP + lit<"(">)+some(
                 append<&ast::IncludeDirective::paths>(ref<Expression>),
                 (SP + lit<",">)) +
             (SP + lit<")">)> {};

  struct EvaluateDirective
      : rule<lit<"#"> + KwEvaluate +
             (SP + lit<"(">)+assign<&ast::EvaluateDirective::expr>(ref<Expression>) +
             (SP + lit<")">)> {};

  struct MacroCallDirective
      : rule<lit<"#"> +
             (SP + lit<"@">)+(SP + assign<&ast::MacroCallDirective::name>(ref<Id>)) +
             (SP +
              lit<"(">)+many(option(SP + lit<",">) + append<&ast::MacroCallDirective::args>(ref<Expression>)) +
             (SP + lit<")">)+assign<&ast::MacroCallDirective::body>(ref<BlockRule>) +
             End> {};

  // Every built-in directive name is refused here. Without this, `#if(..)#end`
  // reads both as the if-directive and as a custom directive named `if`
  // followed by one named `end`, and the readings multiply across siblings:
  // eight of them took 23 s before, 10 ms after.
  static constexpr auto BuiltinDirectiveName =
      (SP + kw<"if">) | (SP + kw<"elseif">) | (SP + kw<"else">) | (SP + kw<"end">) |
      (SP + kw<"set">) | (SP + kw<"foreach">) | (SP + kw<"break">) | (SP + kw<"stop">) |
      (SP + kw<"macro">) | (SP + kw<"parse">) | (SP + kw<"define">) |
      (SP + kw<"include">) | (SP + kw<"evaluate">);

  // A custom directive's name and parenthesized arguments, with no body:
  // Velocity spells a body-taking call `#@d(..)body#end`, so the bodyless
  // form never takes one. `macro_directive.vm` needs exactly this to
  // terminate.
  struct CustomDirective
      : rule<lit<"#"> + !BuiltinDirectiveName +
             (SP + assign<&ast::CustomDirective::name>(ref<Id>)) +
             (((SP + lit<"(">) + many(option(SP + lit<",">) +
                        append<&ast::CustomDirective::args>(ref<Expression>)) +
                    SP + lit<")">) | !(SP + lit<"(">))> {};

  static constexpr auto DirectiveChoice =
      ref<SetDirective> | ref<IfDirective> |
      ref<ForeachDirective> | ref<BreakDirective> |
      ref<StopDirective> | ref<MacroDirective> |
      ref<ParseDirective> | ref<DefineDirective> |
      ref<IncludeDirective> | ref<EvaluateDirective> |
      ref<MacroCallDirective> | ref<CustomDirective>;

  // `.`, `[`, `(` alone (not opening a reference's chain): the same
  // literals `ChainStep` matches, so the lexer never has to choose between
  // two terminals for one byte -- only the parser state does.
  static constexpr auto AtomChoice =
      append<&ast::Block::atoms>(ref<FormalRef>) |
      append<&ast::Block::atoms>(ref<VariableRefChain>) |
      append<&ast::Block::atoms>(DirectiveChoice) |
      append<&ast::Block::atoms>(ref<TEXT> | ref<ESCAPED_CHAR> | ref<ESCAPED_BLOCK> |
                                 ref<LiteralDollar> | ref<LiteralHash> | lit<"\\", !any> |
                                 lit<"."> | lit<"["> | lit<"(">);


  struct BlockRule : rule<many(AtomChoice)> {};

  using entry = BlockRule;
  using ignored = list<>;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace velocity
