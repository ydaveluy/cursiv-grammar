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
using StringConv = cursiv::text_conv<unquote_string>;

inline constexpr auto unescape_block = [](std::string_view t) {
  return t.substr(3, t.size() - 6);
};
using EscapedBlockConv = cursiv::text_conv<unescape_block>;

struct Velocity : cursiv::GrammarSpec {
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
      : terminal<through(lit<"#[[">, lit<"]]#">), EscapedBlockConv> {};

  struct DOLLAR : terminal<lit<"$">> {};
  struct DOLLAR_EXCL : terminal<lit<"$"> + many(lit<"\\">) + lit<"!">> {};
  struct DOLLAR_OBRACE : terminal<lit<"${">> {};
  struct DOLLAR_EXCL_OBRACE
      : terminal<lit<"$"> + many(lit<"\\">) + lit<"!{">> {};

  struct Id : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_-">)> {};

  struct STRING
      : terminal<(lit<"\""> + many(lit<"\"\""> | cr<R"(^"\r\n)">) + lit<"\"">) |
                     (lit<"'"> + many(lit<"''"> | cr<R"(^'\r\n)">) + lit<"'">),
                 StringConv> {};

  struct INTEGER : terminal<some(d), std::int64_t> {};

  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct FLOAT
      : terminal<(many(d) + lit<"."> + some(d) + option(Exponent)) |
                     (some(d) + lit<".", !lit<".">> + option(Exponent)) |
                     (some(d) + Exponent),
                 double> {};

  // One token per operation, value-carrying: the infix table names them by
  // what they mean. A relational/logical operator has both a symbol and a
  // word spelling (`==`/`eq`, `&&`/`and`); either lexes the same value.
  struct MUL : rule<option(ref<WS>) + (lit<"*">), value<ast::Operator::mul>> {};
  struct DIV : rule<option(ref<WS>) + (lit<"/">), value<ast::Operator::div>> {};
  struct MOD : rule<option(ref<WS>) + (lit<"%">), value<ast::Operator::mod>> {};
  struct ADD : rule<option(ref<WS>) + (lit<"+">), value<ast::Operator::add>> {};
  struct SUB : rule<option(ref<WS>) + (lit<"-">), value<ast::Operator::sub>> {};
  struct EQ : rule<option(ref<WS>) + (lit<"==">), value<ast::Operator::eq>> {};
  struct NE : rule<option(ref<WS>) + (lit<"!=">), value<ast::Operator::ne>> {};
  struct KW_EQ : rule<option(ref<WS>) + (kw<"eq">), value<ast::Operator::eq>> {};
  struct KW_NE : rule<option(ref<WS>) + (kw<"ne">), value<ast::Operator::ne>> {};
  struct LE : rule<option(ref<WS>) + (lit<"<=">), value<ast::Operator::le>> {};
  struct GE : rule<option(ref<WS>) + (lit<">=">), value<ast::Operator::ge>> {};
  struct LT : rule<option(ref<WS>) + (lit<"<">), value<ast::Operator::lt>> {};
  struct GT : rule<option(ref<WS>) + (lit<">">), value<ast::Operator::gt>> {};
  struct KW_LE : rule<option(ref<WS>) + (kw<"le">), value<ast::Operator::le>> {};
  struct KW_GE : rule<option(ref<WS>) + (kw<"ge">), value<ast::Operator::ge>> {};
  struct KW_LT : rule<option(ref<WS>) + (kw<"lt">), value<ast::Operator::lt>> {};
  struct KW_GT : rule<option(ref<WS>) + (kw<"gt">), value<ast::Operator::gt>> {};
  struct AND : rule<option(ref<WS>) + (lit<"&&">), value<ast::Operator::logical_and>> {
  };
  struct OR : rule<option(ref<WS>) + (lit<"||">), value<ast::Operator::logical_or>> {};
  struct KW_AND
      : rule<option(ref<WS>) + (kw<"and">), value<ast::Operator::logical_and>> {};
  struct KW_OR : rule<option(ref<WS>) + (kw<"or">), value<ast::Operator::logical_or>> {
  };
  struct RANGE : rule<option(ref<WS>) + (lit<"..">), value<ast::Operator::range>> {};

  struct NOT_OP : rule<option(ref<WS>) + (lit<"!"> | kw<"not">),
                       value<ast::Operator::logical_not>> {};
  struct NEG_OP : rule<option(ref<WS>) + (lit<"-">), value<ast::Operator::negate>> {};

  struct Expression;

  static constexpr auto SP = option(ref<WS>);

  // A property/index/call access, wrapping whatever the chain has built so
  // far as `object` -- a left fold, the way stringtemplate's MemberExpr
  // reads a `.field`/`.( )` chain. `gap::none` before each opener: the
  // reference reads a chain step glued to its base, with no trivia (not
  // even a blank line) between them -- without it, a lone `$x.` at the end
  // of a paragraph reaches across the gap and grabs the next paragraph's
  // first word as a property name.
  static constexpr auto ChainStep =
      nest<ast::PropertyAccess, &ast::PropertyAccess::object>(
          gap::none + lit<"."> + gap::none +
          assign<&ast::PropertyAccess::name>(ref<Id>)) |
      nest<ast::IndexAccess, &ast::IndexAccess::object>(
          gap::none + lit<"["> + assign<&ast::IndexAccess::index>(ref<Expression>) +
          (SP + lit<"]">)) |
      nest<ast::CallAccess, &ast::CallAccess::object>(
          gap::none + lit<"("> +
          option(
              some(append<&ast::CallAccess::args>(ref<Expression>), (SP + lit<",">))) +
          (SP + lit<")">));

  // The bare base of a reference, no chain: `#foreach` reads exactly this
  // for its loop variable.
  struct VariableRefBase
      : rule<(flag<&ast::VariableRef::quiet>(ref<DOLLAR_EXCL>) |
              ref<DOLLAR>) +
             assign<&ast::VariableRef::name>(ref<Id>)> {};

  // A whole reference, chain included -- named so a heterogeneous
  // `append<>` (Block::atoms) can wrap it alone: wrapping the base+chain
  // SEQUENCE directly in `append<>` compiles but fails at parse time
  // ("the AST could not be materialised from this node") on any input that
  // takes a chain step, matching the documented `append<>`-into-a-
  // heterogeneous-list trap.
  struct VariableRefChain
      : rule<ref<VariableRefBase> + many(ChainStep)> {};

  struct FormalRefBase
      : rule<(flag<&ast::FormalRef::quiet>(ref<DOLLAR_EXCL_OBRACE>) |
              ref<DOLLAR_OBRACE>) +
             assign<&ast::FormalRef::name>(ref<Id>)> {};

  struct FormalRef : rule<ref<FormalRefBase> + many(ChainStep) + (SP + lit<"}">)> {};

  struct IdExprRule : rule<(SP + assign<&ast::IdExpr::name>(ref<Id>))> {};
  struct StringLitRule : rule<(SP + assign<&ast::StringLit::value>(ref<STRING>))> {};
  struct IntLitRule : rule<(SP + assign<&ast::IntLit::value>(ref<INTEGER>))> {};
  struct FloatLitRule : rule<(SP + assign<&ast::FloatLit::value>(ref<FLOAT>))> {};

  struct ListLit
      : rule<create<ast::ListLit> +
             (SP + lit<"[">)+option(
                 some(append<&ast::ListLit::items>(ref<Expression>), (SP + lit<",">))) +
             (SP + lit<"]">)> {};

  struct MapEntryRule
      : rule<assign<&ast::MapEntry::key>(ref<Expression>) +
             (SP + lit<":">)+assign<&ast::MapEntry::value>(ref<Expression>)> {};

  struct MapLit
      : rule<create<ast::MapLit> +
             (SP + lit<"{">)+option(some(
                 append<&ast::MapLit::entries>(ref<MapEntryRule>), (SP + lit<",">))) +
             (SP + lit<"}">)> {};

  struct Primary
      : rule<((SP + lit<"(">)+ref<Expression> + (SP + lit<")">)) | ref<ListLit> |
             ref<MapLit> | (SP + ref<VariableRefBase>) | ref<StringLitRule> | ref<IntLitRule> |
             ref<FloatLitRule> | ref<IdExprRule>> {};

  // Unary `!`/`not`/`-`: tighter than every infix level, so it wraps the
  // primary directly rather than sitting in the infix table. The chained
  // and bare readings are separate alternatives -- each manages its own
  // prefix -- rather than one alternative ending in `many(ChainStep)`: a
  // shared prefix feeding two different built classes fails at parse time
  // ("the AST could not be materialised from this node") on whichever
  // input takes the other class.
  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
                 (SP + assign<&ast::UnaryExpr::op>(ref<NOT_OP> | ref<NEG_OP>)) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>)) |
             (ref<Primary> + some(ChainStep)) | ref<Primary>> {};

  // Precedence as VTL 2.0 defines it, tightest to loosest; `..` (range) is
  // loosest.
  struct Expression
      : infix<UnaryExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<MUL> | ref<DIV> | ref<MOD>) >> left(ref<ADD> | ref<SUB>) >>
               left(ref<LE> | ref<GE> | ref<LT> | ref<GT> | ref<KW_LE> | ref<KW_GE> |
                    ref<KW_LT> | ref<KW_GT>) >>
               left(ref<EQ> | ref<NE> | ref<KW_EQ> | ref<KW_NE>) >>
               left(ref<AND> | ref<KW_AND>) >> left(ref<OR> | ref<KW_OR>) >>
               left(ref<RANGE>))> {};

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

  struct IfBranchRule
      : rule<lit<"#"> + KwElseif +
             (SP + lit<"(">)+assign<&ast::IfBranch::cond>(ref<Expression>) +
             (SP + lit<")">)+assign<&ast::IfBranch::body>(ref<BlockRule>)> {};

  struct IfDirectiveRule
      : rule<lit<"#"> + KwIf +
             (SP + lit<"(">)+assign<&ast::IfDirective::cond>(ref<Expression>) +
             (SP + lit<")">)+assign<&ast::IfDirective::thenBlock>(ref<BlockRule>) +
             many(append<&ast::IfDirective::elseifs>(ref<IfBranchRule>)) +
             option(lit<"#"> + KwElse +
                    assign<&ast::IfDirective::elseBlock>(ref<BlockRule>)) +
             End> {};

  struct ForeachDirectiveRule
      : rule<lit<"#"> + KwForeach +
             (SP + lit<"(">) + SP + assign<&ast::ForeachDirective::var>(ref<VariableRefBase>) +
             (SP + kw<"in">)+assign<&ast::ForeachDirective::iterable>(ref<Expression>) +
             (SP + lit<")">)+assign<&ast::ForeachDirective::body>(ref<BlockRule>) +
             End> {};

  struct BreakDirectiveRule
      : rule<create<ast::BreakDirective> + lit<"#"> + KwBreak> {};

  struct StopDirectiveRule
      : rule<create<ast::StopDirective> + lit<"#"> + KwStop> {};

  struct SetDirectiveRule
      : rule<lit<"#"> + KwSet +
             (SP + lit<"(">)+assign<&ast::SetDirective::target>(ref<Expression>) +
             (SP + lit<"=">)+assign<&ast::SetDirective::value>(ref<Expression>) +
             (SP + lit<")">)> {};

  struct MacroDirectiveRule
      : rule<lit<"#"> + KwMacro +
             (SP +
              lit<"(">)+many(append<&ast::MacroDirective::params>(ref<Expression>)) +
             (SP + lit<")">)+assign<&ast::MacroDirective::body>(ref<BlockRule>) + End> {
  };

  struct ParseDirectiveRule
      : rule<lit<"#"> + KwParse +
             (SP + lit<"(">)+assign<&ast::ParseDirective::path>(ref<Expression>) +
             (SP + lit<")">)> {};

  struct DefineDirectiveRule
      : rule<lit<"#"> + KwDefine +
             (SP + lit<"(">)+assign<&ast::DefineDirective::target>(ref<Expression>) +
             (SP + lit<")">)+assign<&ast::DefineDirective::body>(ref<BlockRule>) +
             End> {};

  struct IncludeDirectiveRule
      : rule<lit<"#"> + KwInclude +
             (SP + lit<"(">)+some(
                 append<&ast::IncludeDirective::paths>(ref<Expression>),
                 (SP + lit<",">)) +
             (SP + lit<")">)> {};

  struct EvaluateDirectiveRule
      : rule<lit<"#"> + KwEvaluate +
             (SP + lit<"(">)+assign<&ast::EvaluateDirective::expr>(ref<Expression>) +
             (SP + lit<")">)> {};

  struct MacroCallDirectiveRule
      : rule<lit<"#"> +
             (SP + lit<"@">)+(SP + assign<&ast::MacroCallDirective::name>(ref<Id>)) +
             (SP +
              lit<"(">)+many(append<&ast::MacroCallDirective::args>(ref<Expression>)) +
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
  struct CustomDirectiveRule
      : rule<lit<"#"> + !BuiltinDirectiveName +
             (SP + assign<&ast::CustomDirective::name>(ref<Id>)) +
             option((SP + lit<"(">)+many(
                        append<&ast::CustomDirective::args>(ref<Expression>)) +
                    (SP + lit<")">))> {};

  static constexpr auto DirectiveChoice =
      ref<SetDirectiveRule> | ref<IfDirectiveRule> |
      ref<ForeachDirectiveRule> | ref<BreakDirectiveRule> |
      ref<StopDirectiveRule> | ref<MacroDirectiveRule> |
      ref<ParseDirectiveRule> | ref<DefineDirectiveRule> |
      ref<IncludeDirectiveRule> | ref<EvaluateDirectiveRule> |
      ref<MacroCallDirectiveRule> | ref<CustomDirectiveRule>;

  // `.`, `[`, `(` alone (not opening a reference's chain): the same
  // literals `ChainStep` matches, so the lexer never has to choose between
  // two terminals for one byte -- only the parser state does.
  // The chained and bare readings of a variable are separate alternatives
  // -- see UnaryExpr for why a shared prefix ending in `many(ChainStep)`
  // is refused at parse time instead.
  static constexpr auto AtomChoice =
      append<&ast::Block::atoms>(ref<TEXT> | ref<ESCAPED_CHAR> | ref<ESCAPED_BLOCK> |
                                 lit<"."> | lit<"["> | lit<"(">) |
      append<&ast::Block::atoms>(ref<FormalRef>) |
      append<&ast::Block::atoms>(ref<VariableRefChain>) |
      append<&ast::Block::atoms>(DirectiveChoice);

  struct BlockRule : rule<many(AtomChoice)> {};

  using entry = BlockRule;
  using ignore = list<>;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace velocity
