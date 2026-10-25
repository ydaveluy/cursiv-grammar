// The promql language: its grammar.
#pragma once

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <string>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "promql/core/ast.hpp"

namespace promql {

inline constexpr auto unescape_string =
    [](std::string_view t, std::pmr::memory_resource* mem) -> std::string_view {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (t.front() == '`' || body.find('\\') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  const auto digit = [](char c) -> unsigned {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 16;
  };
  const auto scalar = [&](std::uint32_t c) {
    if (c < 0x80) room[n++] = static_cast<char>(c);
    else {
      const unsigned bytes = c < 0x800 ? 2 : c < 0x10000 ? 3 : 4;
      room[n++] = static_cast<char>((0xffu << (8 - bytes)) | (c >> (6 * (bytes - 1))));
      for (unsigned j = bytes - 1; j > 0; --j)
        room[n++] = static_cast<char>(0x80 | ((c >> (6 * (j - 1))) & 63));
    }
  };
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] != '\\' || i + 1 == body.size()) {
      room[n++] = body[i];
      continue;
    }
    const char e = body[++i];
    switch (e) {
      case 'a': room[n++] = '\a'; break;
      case 'b': room[n++] = '\b'; break;
      case 'f': room[n++] = '\f'; break;
      case 'n': room[n++] = '\n'; break;
      case 'r': room[n++] = '\r'; break;
      case 't': room[n++] = '\t'; break;
      case 'v': room[n++] = '\v'; break;
      case '\\': case '\'': case '"': room[n++] = e; break;
      default: {
        const bool octal = e >= '0' && e <= '7';
        const unsigned count = octal ? 2 : e == 'x' ? 2 : e == 'u' ? 4 : e == 'U' ? 8 : 0;
        std::uint32_t c = octal ? e - '0' : 0;
        if (count == 0 || body.size() - i - 1 < count) return body;
        for (unsigned j = 0; j < count; ++j) {
          const unsigned d = digit(body[++i]);
          if (d >= (octal ? 8u : 16u)) return body;
          c = c * (octal ? 8 : 16) + d;
        }
        if (octal || e == 'x') {
          if (c > 255) return body;
          room[n++] = static_cast<char>(c);
        } else {
          if (c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff)) return body;
          scalar(c);
        }
      }
    }
  }
  return std::string_view(room.data(), n);
};
using Unescaped = cursiv::text_conv<unescape_string>;

inline constexpr auto number_value = [](std::string_view text) {
  std::string digits;
  for (const char c : text) if (c != '_') digits += c;
  std::string_view body = digits;
  int base = 10;
  if (body.starts_with("0x") || body.starts_with("0X")) {
    body.remove_prefix(2);
    base = 16;
  } else if (body.size() > 1 && body.front() == '0') base = 8;
  if (base != 10) {
    std::int64_t value = 0;
    const auto [end, ec] = std::from_chars(body.data(), body.data() + body.size(), value, base);
    if (ec == std::errc{} && end == body.data() + body.size())
      return cursiv::ConversionResult<double>::ok(static_cast<double>(value));
  }
  return cursiv::default_convert<double>(digits);
};
using NumberValue = cursiv::text_conv<number_value>;

struct Promql : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"(\r\t\n )">)> {};
  struct SL_COMMENT
      : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  // Colon joins the class: a recording-rule name (`instance:node_cpu:rate5m`)
  // starts like a keyword may, and the boundary must extend that far for a
  // keyword's guard to yield to the longer metric name under maximal-munch
  // lexing.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_:">;

  // A label name without a colon is also a metric name, and longest-match
  // lexing always resolves that spelling to this wider class: folded into
  // one terminal, same acceptance.
  struct METRIC_NAME
      : terminal<cr<"a-zA-Z_:"> + many(cr<"a-zA-Z0-9_:">)> {};

  static constexpr auto Digits = d + many(d | lit<"_">);
  struct NUMBER : terminal<
      (lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F_">)) |
      (((Digits + option(lit<"."> + option(Digits))) | (lit<"."> + Digits)) +
       option(cr<"eE"> + option(cr<"+-">) + Digits)) |
      ikw<"inf"> | ikw<"nan">> {};
  struct NumericValue : rule<ref<NUMBER>, NumberValue> {};

  struct STRING
      : terminal<(lit<"'"> + many(lit<"\\"> + any | cr<R"(^'\\)">) + lit<"'">) |
                     (lit<"\""> + many(lit<"\\"> + any | cr<R"(^"\\)">) + lit<"\"">) |
                     (lit<"`"> + many(cr<"^`">) + lit<"`">), Unescaped> {};

  // Whether the units run longest-to-shortest is a later check, not a
  // lexical one -- unenforced here too, same acceptance.
  struct DURATION
      : terminal<some(some(d) + ((cr<"mM"> + cr<"sS">) | cr<"smhdwySMHDWY">))> {
  };

  struct SignedAtom;

  struct ADD : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct SUB : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct MULT : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIV : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct MOD : terminal<lit<"%">, value<ast::Operator::modulo>> {};
  struct POW : terminal<lit<"^">, value<ast::Operator::pow>> {};

  struct EQ : terminal<lit<"=">, value<ast::Operator::eq>> {};
  struct DEQ : terminal<lit<"==">, value<ast::Operator::deq>> {};
  struct NE : terminal<lit<"!=">, value<ast::Operator::ne>> {};
  struct GT : terminal<lit<">">, value<ast::Operator::gt>> {};
  struct LT : terminal<lit<"<">, value<ast::Operator::lt>> {};
  struct GE : terminal<lit<">=">, value<ast::Operator::ge>> {};
  struct LE : terminal<lit<"<=">, value<ast::Operator::le>> {};
  struct RE : terminal<lit<"=~">, value<ast::Operator::re>> {};
  struct NRE : terminal<lit<"!~">, value<ast::Operator::nre>> {};

  struct TRIM_UPPER : terminal<lit<"</">, value<ast::Operator::trim_upper>> {};
  struct TRIM_LOWER : terminal<lit<">/">, value<ast::Operator::trim_lower>> {};

  struct AT : terminal<lit<"@">, value<ast::Operator::at>> {};

  // Plain text (no operator value): each is also a legal label name, so it
  // is spelled once and referenced from both positions -- a second terminal
  // for one spelling would give the lexer two tokens to choose between
  // before the parser ever sees them.
  struct AND : terminal<ikw<"and">> {};
  struct OR : terminal<ikw<"or">> {};
  struct UNLESS : terminal<ikw<"unless">> {};
  struct BY : terminal<ikw<"by">> {};
  struct WITHOUT : terminal<ikw<"without">> {};
  struct ON : terminal<ikw<"on">> {};
  struct IGNORING : terminal<ikw<"ignoring">> {};
  struct GROUP_LEFT : terminal<ikw<"group_left">> {};
  struct GROUP_RIGHT : terminal<ikw<"group_right">> {};
  struct OFFSET : terminal<ikw<"offset">> {};
  struct ATAN2 : terminal<ikw<"atan2">> {};
  struct BOOL : terminal<ikw<"bool">> {};

  struct AGGREGATION_OPERATOR
      : terminal<ikw<"sum"> | ikw<"min"> | ikw<"max"> | ikw<"avg"> |
                 ikw<"group"> | ikw<"stddev"> | ikw<"stdvar"> |
                 ikw<"count_values"> | ikw<"count"> | ikw<"bottomk"> |
                 ikw<"topk"> | ikw<"quantile"> | ikw<"limitk"> | ikw<"limit_ratio">> {};

  // A binary level's operator, carried as a value rather than as the text
  // that spells it: `and`/`AND` are one answer downstream.
  struct AndOp : rule<ref<AND>, value<ast::Operator::logical_and>> {};
  struct UnlessOp : rule<ref<UNLESS>, value<ast::Operator::logical_unless>> {
  };
  struct OrOp : rule<ref<OR>, value<ast::Operator::logical_or>> {};
  struct Atan2Op : rule<ref<ATAN2>, value<ast::Operator::atan2>> {};

  struct Expression;
  struct UnaryExprRule;

  struct LabelName
      : rule<ref<METRIC_NAME> | ref<AND> | ref<OR> | ref<UNLESS> | ref<BY> |
             ref<WITHOUT> | ref<ON> | ref<IGNORING> | ref<GROUP_LEFT> |
             ref<GROUP_RIGHT> | ref<OFFSET> | ref<BOOL> |
             ref<AGGREGATION_OPERATOR> | ref<ATAN2>> {};

  struct LabelMatcherRule
      : rule<create<ast::LabelMatcher>(
            assign<&ast::LabelMatcher::name>(ref<LabelName> | ref<STRING> | ref<NUMBER>) +
            assign<&ast::LabelMatcher::op>(ref<EQ> | ref<NE> | ref<RE> |
                                           ref<NRE>) +
            assign<&ast::LabelMatcher::value>(ref<STRING>))> {};

  struct QuotedMetricRule
      : rule<create<ast::LabelMatcher>(assign<&ast::LabelMatcher::value>(ref<STRING>))> {};

  struct GroupingRule
      : rule<create<ast::Grouping>(
            (flag<&ast::Grouping::ignoring>(ref<IGNORING>) | ref<ON>) +
            lit<"("> +
            many(append<&ast::Grouping::labels>(ref<LabelName> | ref<STRING> | ref<NUMBER>), lit<",">) +
            option(lit<",">) +
            lit<")"> +
            option(flag<&ast::Grouping::hasSide>(
                (flag<&ast::Grouping::groupRight>(ref<GROUP_RIGHT>) |
                 ref<GROUP_LEFT>) +
                option(lit<"("> +
                       many(append<&ast::Grouping::sideLabels>(
                                ref<LabelName> | ref<STRING> | ref<NUMBER>),
                            lit<",">) + option(lit<",">) +
                       lit<")">))))> {};

  // One home for instantSelector, matrixSelector and offset -- see
  // ast::Selector.
  struct SelectorRule
      : rule<create<ast::Selector>(
            ((assign<&ast::Selector::metric>(ref<LabelName>) +
              option(lit<"{"> +
                     option(some(append<&ast::Selector::matchers>(
                                     ref<LabelMatcherRule> | ref<QuotedMetricRule>),
                                 lit<",">) +
                            option(lit<",">)) +
                     lit<"}">)) |
             (lit<"{"> +
              many(append<&ast::Selector::matchers>(ref<LabelMatcherRule> | ref<QuotedMetricRule>),
                   lit<",">) +
              option(lit<",">) + lit<"}">)) +
            option(lit<"["> + assign<&ast::Selector::range>(ref<Expression>) +
                   lit<"]">) +
            option(ref<OFFSET> +
                   assign<&ast::Selector::offset>(ref<SignedAtom>)))> {};

  struct FunctionCallRule
      : rule<create<ast::FunctionCall>(
            assign<&ast::FunctionCall::name>(ref<METRIC_NAME>) + lit<"("> +
            many(append<&ast::FunctionCall::args>(ref<Expression>),
                 lit<",">) +
            lit<")">)> {};

  static constexpr auto AggregationGrouping =
      flag<&ast::AggregationExpr::hasGrouping>(
          (flag<&ast::AggregationExpr::without>(ref<WITHOUT>) |
           ref<BY>) +
          lit<"("> +
          many(append<&ast::AggregationExpr::groupBy>(ref<LabelName> | ref<STRING> | ref<NUMBER>),
               lit<",">) + option(lit<",">) +
          lit<")">);

  struct AggregationRule
      : rule<create<ast::AggregationExpr>(
            assign<&ast::AggregationExpr::op>(ref<AGGREGATION_OPERATOR>) +
            option(AggregationGrouping) + lit<"("> +
            many(append<&ast::AggregationExpr::args>(ref<Expression>), lit<",">) +
            lit<")"> + option(AggregationGrouping))> {};

  struct Atom
      : rule<ref<FunctionCallRule>  //
             | ref<AggregationRule>  //
             | ref<SelectorRule>  //
             | create<ast::DurationLit>(assign<&ast::DurationLit::text>(ref<DURATION>))
             | create<ast::NumberLit>(
                   assign<&ast::NumberLit::value>(ref<NumericValue>))  //
             | create<ast::StringLit>(
                   assign<&ast::StringLit::value>(ref<STRING>))  //
             | (lit<"("> + ref<Expression> + lit<")">)> {};

  // Postfix: `expr[range:step]offset?`, chained since a PromQL subquery
  // operation can itself be the operand of another subquery.
  struct SignedAtom
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<ADD> | ref<SUB>) +
                 assign<&ast::UnaryExpr::operand>(ref<Atom>)) | ref<Atom>> {};
  struct Timestamp : rule<create<ast::TimestampBoundary>(
                              assign<&ast::TimestampBoundary::name>(
                                  ikw<"start"> | ikw<"end">) +
                              lit<"("> + lit<")">) | ref<SignedAtom>> {};
  struct Subqueried
      : rule<ref<Atom> +
             many(nest<ast::Subquery, &ast::Subquery::operand>(
                      lit<"["> + assign<&ast::Subquery::range>(ref<Expression>) +
                      lit<":"> +
                      option(assign<&ast::Subquery::step>(ref<Expression>)) +
                      lit<"]"> +
                      option(ref<OFFSET> +
                             assign<&ast::Subquery::offset>(ref<SignedAtom>))) |
                  nest<ast::Timestamped, &ast::Timestamped::operand>(
                      ref<AT> +
                      assign<&ast::Timestamped::timestamp>(ref<Timestamp>) +
                      option(ref<OFFSET> + assign<&ast::Timestamped::offset>(
                                               ref<SignedAtom>))) |
                  nest<ast::RangeModifier, &ast::RangeModifier::operand>(
                      assign<&ast::RangeModifier::name>(ikw<"anchored"> | ikw<"smoothed">)))> {};

  struct FillRule
      : rule<assign<&ast::FillModifier::name>(
                 ikw<"fill"> | ikw<"fill_left"> | ikw<"fill_right">) +
             lit<"("> + assign<&ast::FillModifier::value>(ref<SignedAtom>) + lit<")">> {};
  static constexpr auto BinaryModifiers =
      option(flag<&ast::BinaryExpr::boolMod>(ref<BOOL>)) +
      option(assign<&ast::BinaryExpr::matching>(ref<GroupingRule>)) +
      many(append<&ast::BinaryExpr::fills>(ref<FillRule>));

  // Right-associative, tighter than unary: `a^b^c` is `a^(b^c)`.
  struct PowExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<Subqueried>) +
                 assign<&ast::BinaryExpr::op>(ref<POW>) +
                 BinaryModifiers +
                 assign<&ast::BinaryExpr::right>(ref<UnaryExprRule>))  //
             | ref<Subqueried>> {};

  // Prefix, binds tighter than `*`/`/`/`%` and looser than `^`: `-a^2` is
  // `-(a^2)`, `-a*b` is `(-a)*b`.
  struct UnaryExprRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<ADD> | ref<SUB>) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExprRule>))  //
             | ref<PowExpr>> {};

  struct MultExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<MultExpr>) +
                 assign<&ast::BinaryExpr::op>(ref<MULT> | ref<DIV> |
                                              ref<MOD> | ref<Atan2Op>) +
                 BinaryModifiers +
                 assign<&ast::BinaryExpr::right>(ref<UnaryExprRule>))  //
             | ref<UnaryExprRule>> {};

  struct AddExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<AddExpr>) +
                 assign<&ast::BinaryExpr::op>(ref<ADD> | ref<SUB>) +
                 BinaryModifiers +
                 assign<&ast::BinaryExpr::right>(ref<MultExpr>))  //
             | ref<MultExpr>> {};

  struct CompareExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<CompareExpr>) +
                 assign<&ast::BinaryExpr::op>(ref<DEQ> | ref<NE> | ref<GT> |
                                              ref<LT> | ref<GE> | ref<LE> | ref<TRIM_UPPER> | ref<TRIM_LOWER>) +
                 BinaryModifiers +
                 assign<&ast::BinaryExpr::right>(ref<AddExpr>))  //
             | ref<AddExpr>> {};

  struct AndUnlessExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<AndUnlessExpr>) +
                 assign<&ast::BinaryExpr::op>(ref<AndOp> | ref<UnlessOp>) +
                 BinaryModifiers +
                 assign<&ast::BinaryExpr::right>(ref<CompareExpr>))  //
             | ref<CompareExpr>> {};

  struct OrExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<OrExpr>) +
                 assign<&ast::BinaryExpr::op>(ref<OrOp>) +
                 BinaryModifiers +
                 assign<&ast::BinaryExpr::right>(ref<AndUnlessExpr>))  //
             | ref<AndUnlessExpr>> {};

  // Loosest: the `@` modifier. A bare `on`/`ignoring` treated as its own
  // operator is dropped -- it would make UNLESS reachable through two
  // derivations of the same input (it is already `AndUnlessExpr`'s
  // operator), the redundant-derivation shape that forks a parse at every
  // occurrence, and bare `on`/`ignoring` with no preceding operator names
  // no real PromQL construct. Every real binary level already carries
  // `grouping?` on its own.
  struct Expression : rule<ref<OrExpr>> {};

  struct QueryRule
      : rule<create<ast::Query>(
            assign<&ast::Query::expression>(ref<Expression>))> {};

  using entry = QueryRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT>;
};

}  // namespace promql
