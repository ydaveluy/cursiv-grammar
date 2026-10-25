// The caql language: its grammar.
#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "caql/core/ast.hpp"

namespace caql {

inline constexpr auto unescape_string =
    [](std::string_view t, std::pmr::memory_resource* mem) -> std::string_view {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('\\') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    char c = body[i];
    if (c == '\\' && i + 1 < body.size()) {
      ++i;
      switch (body[i]) {
        case 'n': c = '\n'; break;
        case 't': c = '\t'; break;
        case 'r': c = '\r'; break;
        default: c = body[i]; break;
      }
    }
    room[n++] = c;
  }
  return std::string_view(room.data(), n);
};
using Unescaped = cursiv::text_conv<unescape_string>;

struct Caql : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"(\r\t\n )">)> {};

  // NAME is lowercase-only and carries a colon, so a keyword's guard must
  // extend across both or it fires inside a longer name (`and:foo`).
  static constexpr auto word_boundary = cr<"a-z0-9_:">;

  struct NAME : terminal<cr<"a-z_:"> + many(cr<"a-z0-9_:">)> {};

  static constexpr auto NumeralBody = some(d) + option(lit<"."> + some(d));
  struct NUMBER
      : terminal<NumeralBody + option(lit<"e"> + option(cr<"-+">) + NumeralBody),
                 double> {};

  struct STRING
      : terminal<(lit<"'"> + many((lit<"\\"> + any) | cr<R"(^'\\)">) + lit<"'">) |
                     (lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                      lit<"\"">),
                 Unescaped> {};

  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  struct UUID
      : terminal<repeat<8>(HexDigit) + lit<"-"> + repeat<4>(HexDigit) +
                     lit<"-"> + repeat<4>(HexDigit) + lit<"-"> +
                     repeat<4>(HexDigit) + lit<"-"> + repeat<12>(HexDigit)> {
  };

  struct DURATION : terminal<some(d) + cr<"smhdwy">> {};

  struct ADD : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct SUB : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct MULT : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIV : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct MOD : terminal<lit<"%">, value<ast::Operator::modulo>> {};
  struct POW : terminal<lit<"^">, value<ast::Operator::pow>> {};

  struct DEQ : terminal<lit<"==">, value<ast::Operator::deq>> {};
  struct NE : terminal<lit<"!=">, value<ast::Operator::ne>> {};
  struct GT : terminal<lit<">">, value<ast::Operator::gt>> {};
  struct LT : terminal<lit<"<">, value<ast::Operator::lt>> {};
  struct GE : terminal<lit<">=">, value<ast::Operator::ge>> {};
  struct LE : terminal<lit<"<=">, value<ast::Operator::le>> {};

  // Plain text (no operator value): each is also a legal name, so it is
  // spelled once and referenced from both positions.
  struct AND : terminal<kw<"and">> {};
  struct OR : terminal<kw<"or">> {};
  struct UNLESS : terminal<kw<"unless">> {};
  struct BY : terminal<kw<"by">> {};
  struct WITHOUT : terminal<kw<"without">> {};
  struct ON : terminal<kw<"on">> {};
  struct IGNORING : terminal<kw<"ignoring">> {};
  struct GROUP_LEFT : terminal<kw<"group_left">> {};
  struct GROUP_RIGHT : terminal<kw<"group_right">> {};
  struct OFFSET : terminal<kw<"offset">> {};
  struct BOOL : terminal<kw<"bool">> {};

  // `[a-z_:][a-z0-9_:]*` already excludes `=`, so no word-boundary guard
  // is needed to keep this out of a name.
  struct LIMIT : terminal<lit<"limit=">> {};

  struct AndOp : rule<ref<AND>, value<ast::Operator::logical_and>> {};
  struct UnlessOp : rule<ref<UNLESS>, value<ast::Operator::logical_unless>> {
  };
  struct OrOp : rule<ref<OR>, value<ast::Operator::logical_or>> {};

  struct Expression;

  // A soft keyword: every aggregation/join word is also a legal name.
  struct Name
      : rule<ref<NAME> | ref<AND> | ref<OR> | ref<UNLESS> | ref<BY> |
             ref<WITHOUT> | ref<ON> | ref<IGNORING> | ref<GROUP_LEFT> |
             ref<GROUP_RIGHT> | ref<OFFSET> | ref<BOOL>> {};

  struct NumberLitRule
      : rule<create<ast::NumberLit>(
            assign<&ast::NumberLit::value>(ref<NUMBER>))> {};
  struct StringLitRule
      : rule<create<ast::StringLit>(
            assign<&ast::StringLit::value>(ref<STRING>))> {};
  struct UuidLitRule
      : rule<create<ast::UuidLit>(assign<&ast::UuidLit::value>(ref<UUID>))> {
  };

  struct GroupingRule
      : rule<create<ast::Grouping>(
            (flag<&ast::Grouping::ignoring>(ref<IGNORING>) | ref<ON>) +
            lit<"("> +
            many(append<&ast::Grouping::labels>(ref<Name>), lit<",">) +
            lit<")"> +
            option(flag<&ast::Grouping::hasSide>(
                (flag<&ast::Grouping::groupRight>(ref<GROUP_RIGHT>) |
                 ref<GROUP_LEFT>) +
                option(lit<"("> +
                       many(append<&ast::Grouping::sideLabels>(ref<Name>),
                            lit<",">) +
                       lit<")">))))> {};

  struct BraceExprRule
      : rule<create<ast::BraceExpr>(
            assign<&ast::BraceExpr::name>(ref<Name>) + lit<"{"> +
            some(append<&ast::BraceExpr::operands>(ref<Expression>),
                 lit<",">) +
            lit<"}">)> {};

  struct CallArgFirstRule
      : rule<create<ast::CallArg>(assign<&ast::CallArg::value>(
            ref<NumberLitRule> | ref<StringLitRule> | ref<UuidLitRule>))> {};
  struct CallArgRule
      : rule<create<ast::CallArg>(
            option(flag<&ast::CallArg::limit>(ref<LIMIT>)) +
            assign<&ast::CallArg::value>(
                ref<NumberLitRule> | ref<StringLitRule> |
                ref<UuidLitRule>))> {};

  struct CallExprRule
      : rule<create<ast::CallExpr>(
            assign<&ast::CallExpr::name>(ref<Name>) + lit<"("> +
            append<&ast::CallExpr::args>(ref<CallArgFirstRule>) +
            many(lit<","> +
                 append<&ast::CallExpr::args>(ref<CallArgRule>)) +
            lit<")">)> {};

  struct Atom
      : rule<ref<BraceExprRule>  //
             | ref<CallExprRule>  //
             | ref<NumberLitRule>  //
             | ref<StringLitRule>  //
             | ref<UuidLitRule>  //
             | (lit<"("> + ref<Expression> + lit<")">)> {};

  // Right-associative, tightest: `a^b^c` is `a^(b^c)`.
  struct PowExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<Atom>) +
                 assign<&ast::BinaryExpr::op>(ref<POW>) +
                 option(
                     assign<&ast::BinaryExpr::matching>(ref<GroupingRule>)) +
                 assign<&ast::BinaryExpr::right>(ref<PowExpr>))  //
             | ref<Atom>> {};

  // A subquery range chains: `expr[1h:5m][1d:1h]` applies the second range
  // to the first's own result.
  struct Subqueried
      : rule<ref<PowExpr> +
             many(nest<ast::Subquery, &ast::Subquery::operand>(
                 lit<"["> + assign<&ast::Subquery::range>(ref<DURATION>) +
                 lit<":"> +
                 option(assign<&ast::Subquery::step>(ref<DURATION>)) +
                 lit<"]"> +
                 option(ref<OFFSET> +
                        assign<&ast::Subquery::offset>(ref<DURATION>))))> {};

  // Prefix, binds tighter than `*`/`/`/`%` and looser than `^`.
  struct UnaryExprRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<ADD> | ref<SUB>) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExprRule>))  //
             | ref<Subqueried>> {};

  struct MultExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<MultExpr>) +
                 assign<&ast::BinaryExpr::op>(ref<MULT> | ref<DIV> |
                                              ref<MOD>) +
                 option(
                     assign<&ast::BinaryExpr::matching>(ref<GroupingRule>)) +
                 assign<&ast::BinaryExpr::right>(ref<UnaryExprRule>))  //
             | ref<UnaryExprRule>> {};

  struct AddExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<AddExpr>) +
                 assign<&ast::BinaryExpr::op>(ref<ADD> | ref<SUB>) +
                 option(
                     assign<&ast::BinaryExpr::matching>(ref<GroupingRule>)) +
                 assign<&ast::BinaryExpr::right>(ref<MultExpr>))  //
             | ref<MultExpr>> {};

  struct CompareExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<CompareExpr>) +
                 assign<&ast::BinaryExpr::op>(ref<DEQ> | ref<NE> | ref<GT> |
                                              ref<LT> | ref<GE> | ref<LE>) +
                 option(flag<&ast::BinaryExpr::boolMod>(ref<BOOL>)) +
                 option(
                     assign<&ast::BinaryExpr::matching>(ref<GroupingRule>)) +
                 assign<&ast::BinaryExpr::right>(ref<AddExpr>))  //
             | ref<AddExpr>> {};

  struct AndUnlessExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<AndUnlessExpr>) +
                 assign<&ast::BinaryExpr::op>(ref<AndOp> | ref<UnlessOp>) +
                 option(
                     assign<&ast::BinaryExpr::matching>(ref<GroupingRule>)) +
                 assign<&ast::BinaryExpr::right>(ref<CompareExpr>))  //
             | ref<CompareExpr>> {};

  struct OrExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<OrExpr>) +
                 assign<&ast::BinaryExpr::op>(ref<OrOp>) +
                 option(
                     assign<&ast::BinaryExpr::matching>(ref<GroupingRule>)) +
                 assign<&ast::BinaryExpr::right>(ref<AndUnlessExpr>))  //
             | ref<AndUnlessExpr>> {};

  // Loosest: the `| name(args)` metrics-aggregation pipe, chainable.
  struct Expression
      : rule<create<ast::PipeExpr>(
                 assign<&ast::PipeExpr::operand>(ref<Expression>) +
                 lit<"|"> + assign<&ast::PipeExpr::name>(ref<Name>) +
                 lit<"("> +
                 many(append<&ast::PipeExpr::args>(ref<Expression>),
                      lit<",">) +
                 lit<")">)  //
             | ref<OrExpr>> {};

  struct QueryRule
      : rule<create<ast::Query>(
            assign<&ast::Query::expression>(ref<Expression>))> {};

  using entry = QueryRule;
  using ignore = WS;
};

}  // namespace caql
