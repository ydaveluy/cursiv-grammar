// The promql language: its grammar.
#pragma once

#include <charconv>
#include <cmath>
#include <cstdlib>
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
  auto result=cursiv::default_convert<double>(digits);
  if(result||base==16)return result;
  // Native strconv accepts floating underflow as zero or a subnormal.
  char* end=nullptr;
  const double value=std::strtod(digits.c_str(),&end);
  if(end==digits.c_str()+digits.size()&&!std::isinf(value))
    return cursiv::ConversionResult<double>::ok(value);
  return result;
};

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
      : terminal<!(ikw<"on"> | ikw<"ignoring"> | ikw<"group_left"> | ikw<"group_right"> | ikw<"bool"> | ikw<"atan2">) +
          cr<"a-zA-Z_:"> + many(cr<"a-zA-Z0-9_:">)> {};

  static constexpr auto Digits = d + many(option(lit<"_">) + d);
  static constexpr auto Hex = cr<"0-9a-fA-F">;
  static constexpr auto HexDigits = option(lit<"_">) + Hex + many(option(lit<"_">) + Hex);
  struct NUMBER : terminal<
      (lit<"0"> + cr<"xX"> + HexDigits) |
      (((Digits + option(lit<"."> + option(Digits))) | (lit<"."> + Digits)) +
       option(cr<"eE"> + option(cr<"+-">) + Digits)) |
      ikw<"inf"> | ikw<"nan">> {};
  struct NumericValue : rule<as<number_value>(ref<NUMBER>)> {};

  static constexpr auto scalar4 = (cr<"0-9a-cA-Ce-fE-F"> + repeat<3>(Hex)) |
      (cr<"dD"> + cr<"0-7"> + repeat<2>(Hex));
  static constexpr auto scalar8 = (lit<"0000"> + scalar4) |
      (lit<"000"> + cr<"1-9a-fA-F"> + repeat<4>(Hex)) | (lit<"0010"> + repeat<4>(Hex));
  static constexpr auto common_escape = cr<R"(abfnrtv\\)"> |
      (lit<"x"> + repeat<2>(Hex)) | (lit<"u"> + scalar4) | (lit<"U"> + scalar8) |
      (cr<"0-3"> + repeat<2>(cr<"0-7">));
  struct STRING : terminal<as<unescape_string>(
      (lit<"'"> + many((lit<"\\"> + (common_escape | lit<"'">)) | cr<R"(^'\\\r\n)">) + lit<"'">) |
      (lit<"\""> + many((lit<"\\"> + (common_escape | lit<"\"">)) | cr<R"(^"\\\r\n)">) + lit<"\"">) |
      (lit<"`"> + many(cr<"^`">) + lit<"`">))> {};

  struct DURATION : terminal<some(some(d) + (lit<"ms"> | cr<"smhdwy">))> {};

  struct SignedAtom;

  struct ADD : terminal<as<ast::Operator::add>(lit<"+">)> {};
  struct SUB : terminal<as<ast::Operator::subtract>(lit<"-">)> {};

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

  struct FILL : terminal<ikw<"fill">> {};
  struct FILL_LEFT : terminal<ikw<"fill_left">> {};
  struct FILL_RIGHT : terminal<ikw<"fill_right">> {};
  struct START : terminal<ikw<"start">> {};
  struct END : terminal<ikw<"end">> {};
  struct STEP : terminal<ikw<"step">> {};
  struct RANGE : terminal<ikw<"range">> {};
  struct MIN_OF : terminal<ikw<"min_of">> {};
  struct MAX_OF : terminal<ikw<"max_of">> {};
  struct ANCHORED : terminal<ikw<"anchored">> {};
  struct SMOOTHED : terminal<ikw<"smoothed">> {};

  struct AGGREGATION_OPERATOR
      : terminal<ikw<"sum"> | ikw<"min"> | ikw<"max"> | ikw<"avg"> |
                 ikw<"group"> | ikw<"stddev"> | ikw<"stdvar"> |
                 ikw<"count_values"> | ikw<"count"> | ikw<"bottomk"> |
                 ikw<"topk"> | ikw<"quantile"> | ikw<"limitk"> | ikw<"limit_ratio">> {};

  // A binary level's operator, carried as a value rather than as the text
  // that spells it: `and`/`AND` are one answer downstream.

  struct Expression;
  struct UnaryExpr;

  struct LabelName
      : rule<ref<METRIC_NAME> | ref<AND> | ref<OR> | ref<UNLESS> | ref<BY> |
             ref<WITHOUT> | ref<ON> | ref<IGNORING> | ref<GROUP_LEFT> |
             ref<GROUP_RIGHT> | ref<OFFSET> | ref<BOOL> |
             ref<AGGREGATION_OPERATOR> | ref<ATAN2> | ref<FILL> | ref<FILL_LEFT> | ref<FILL_RIGHT> |
             ref<START> | ref<END> | ref<STEP> | ref<RANGE> | ref<MIN_OF> | ref<MAX_OF> | ref<ANCHORED> | ref<SMOOTHED>> {};
  struct MetricIdentifier : rule<ref<METRIC_NAME> | ref<AND> | ref<OR> | ref<UNLESS> | ref<BY> |
      ref<WITHOUT> | ref<OFFSET> | ref<AGGREGATION_OPERATOR> | ref<FILL> | ref<FILL_LEFT> | ref<FILL_RIGHT> |
      ref<START> | ref<END> | ref<STEP> | ref<RANGE> | ref<MIN_OF> | ref<MAX_OF> | ref<ANCHORED> | ref<SMOOTHED>> {};

  struct LabelMatcher
      : rule<create<ast::LabelMatcher>(
            assign<&ast::LabelMatcher::name>(ref<LabelName> | ref<STRING>) +
            assign<&ast::LabelMatcher::op>(as<ast::Operator::eq>(lit<"=">) | as<ast::Operator::ne>(lit<"!=">) | as<ast::Operator::re>(lit<"=~">) |
                                           as<ast::Operator::nre>(lit<"!~">)) +
            assign<&ast::LabelMatcher::value>(ref<STRING>))> {};

  struct QuotedMetric
      : rule<assign<&ast::LabelMatcher::value>(ref<STRING>)> {};

  struct Grouping
      : rule<create<ast::Grouping>(
            (flag<&ast::Grouping::ignoring>(ref<IGNORING>) | ref<ON>) +
            lit<"("> +
            option(some(append<&ast::Grouping::labels>(ref<LabelName> | ref<STRING>), lit<",">) +
            option(lit<",">)) +
            lit<")"> +
            option(flag<&ast::Grouping::hasSide>(
                (flag<&ast::Grouping::groupRight>(ref<GROUP_RIGHT>) |
                 ref<GROUP_LEFT>) +
                option(lit<"("> +
                       option(some(append<&ast::Grouping::sideLabels>(
                                ref<LabelName> | ref<STRING>),
                            lit<",">) + option(lit<",">)) +
                       lit<")">))))> {};

  // One home for instantSelector, matrixSelector and offset -- see
  // ast::Selector.
  struct Selector
      : rule<create<ast::Selector>(
            ((assign<&ast::Selector::metric>(ref<MetricIdentifier>) +
              option(lit<"{"> +
                     option(some(append<&ast::Selector::matchers>(
                                     ref<LabelMatcher> | ref<QuotedMetric>),
                                 lit<",">) +
                            option(lit<",">)) +
                     lit<"}">)) |
             (lit<"{"> +
              option(some(append<&ast::Selector::matchers>(ref<LabelMatcher> | ref<QuotedMetric>),
                   lit<",">) +
              option(lit<",">)) + lit<"}">)) +
            option(lit<"["> + assign<&ast::Selector::range>(ref<Expression>) +
                   lit<"]">) +
            option(ref<OFFSET> +
                   assign<&ast::Selector::offset>(ref<SignedAtom>)))> {};

  struct FunctionCall
      : rule<create<ast::FunctionCall>(
            assign<&ast::FunctionCall::name>(ref<METRIC_NAME> | ref<START> | ref<END> |
                ref<STEP> | ref<RANGE> | ref<MIN_OF> | ref<MAX_OF>) + lit<"("> +
            many(append<&ast::FunctionCall::args>(ref<Expression>),
                 lit<",">) +
            lit<")">)> {};

  static constexpr auto AggregationGrouping =
      flag<&ast::AggregationExpr::hasGrouping>(
          (flag<&ast::AggregationExpr::without>(ref<WITHOUT>) |
           ref<BY>) +
          lit<"("> +
          option(some(append<&ast::AggregationExpr::groupBy>(ref<LabelName> | ref<STRING>),
               lit<",">) + option(lit<",">)) +
          lit<")">);

  struct Aggregation
      : rule<create<ast::AggregationExpr>(
            assign<&ast::AggregationExpr::op>(ref<AGGREGATION_OPERATOR>) +
            ((AggregationGrouping + lit<"("> +
              many(append<&ast::AggregationExpr::args>(ref<Expression>),lit<",">) + lit<")">) |
             (lit<"("> + many(append<&ast::AggregationExpr::args>(ref<Expression>),lit<",">) +
              lit<")"> + option(AggregationGrouping))))> {};

  struct NumberAtom : rule<create<ast::DurationLit>(assign<&ast::DurationLit::text>(ref<DURATION>)) |
      create<ast::NumberLit>(assign<&ast::NumberLit::value>(ref<NumericValue>))> {};
  struct Atom
      : rule<ref<FunctionCall>  //
             | ref<Aggregation>  //
             | ref<Selector>  //
             | ref<NumberAtom> //
             | create<ast::StringLit>(
                   assign<&ast::StringLit::value>(ref<STRING>))  //
             | (lit<"("> + ref<Expression> + lit<")">)> {};

  // Postfix: `expr[range:step]offset?`, chained since a PromQL subquery
  // operation can itself be the operand of another subquery.
  struct SignedAtom
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<ADD> | ref<SUB>) +
                 assign<&ast::UnaryExpr::operand>(ref<Atom>)) | ref<Atom>> {};
  struct SignedNumberAtom;
  struct Timestamp : rule<create<ast::TimestampBoundary>(
                              assign<&ast::TimestampBoundary::name>(
                                  ref<START> | ref<END>) +
                              lit<"("> + lit<")">) | ref<SignedNumberAtom>> {};
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
                      as<ast::Operator::at>(lit<"@">) +
                      assign<&ast::Timestamped::timestamp>(ref<Timestamp>) +
                      option(ref<OFFSET> + assign<&ast::Timestamped::offset>(
                                               ref<SignedAtom>))) |
                  nest<ast::RangeModifier, &ast::RangeModifier::operand>(
                      assign<&ast::RangeModifier::name>(ref<ANCHORED> | ref<SMOOTHED>)))> {};

  struct SignedNumberAtom : rule<create<ast::UnaryExpr>(
      assign<&ast::UnaryExpr::op>(ref<ADD> | ref<SUB>) +
      assign<&ast::UnaryExpr::operand>(ref<NumberAtom>)) | ref<NumberAtom>> {};
  struct Fill
      : rule<assign<&ast::FillModifier::name>(
                 ref<FILL> | ref<FILL_LEFT> | ref<FILL_RIGHT>) +
             lit<"("> + assign<&ast::FillModifier::value>(ref<SignedNumberAtom>) + lit<")">> {};
  static constexpr auto BinaryModifiers =
      option(flag<&ast::BinaryExpr::boolMod>(ref<BOOL>)) +
      option(assign<&ast::BinaryExpr::matching>(ref<Grouping>)) +
      many(append<&ast::BinaryExpr::fills>(ref<Fill>));

  // Right-associative, tighter than unary: `a^b^c` is `a^(b^c)`.
  struct PowExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<Subqueried>) +
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::pow>(lit<"^">)) +
                 BinaryModifiers +
                 assign<&ast::BinaryExpr::right>(ref<UnaryExpr>))  //
             | ref<Subqueried>> {};

  // Prefix, binds tighter than `*`/`/`/`%` and looser than `^`: `-a^2` is
  // `-(a^2)`, `-a*b` is `(-a)*b`.
  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<ADD> | ref<SUB>) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>))  //
             | ref<PowExpr>> {};

  struct MultExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<MultExpr>) +
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::multiply>(lit<"*">) | as<ast::Operator::divide>(lit<"/">) |
                                              as<ast::Operator::modulo>(lit<"%">) | as<ast::Operator::atan2>(ref<ATAN2>)) +
                 BinaryModifiers +
                 assign<&ast::BinaryExpr::right>(ref<UnaryExpr>))  //
             | ref<UnaryExpr>> {};

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
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::deq>(lit<"==">) | as<ast::Operator::ne>(lit<"!=">) | as<ast::Operator::gt>(lit<">">) |
                                              as<ast::Operator::lt>(lit<"<">) | as<ast::Operator::ge>(lit<">=">) | as<ast::Operator::le>(lit<"<=">) | as<ast::Operator::trim_upper>(lit<"</">) | as<ast::Operator::trim_lower>(lit<">/">)) +
                 BinaryModifiers +
                 assign<&ast::BinaryExpr::right>(ref<AddExpr>))  //
             | ref<AddExpr>> {};

  struct AndUnlessExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<AndUnlessExpr>) +
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::logical_and>(ref<AND>) | as<ast::Operator::logical_unless>(ref<UNLESS>)) +
                 BinaryModifiers +
                 assign<&ast::BinaryExpr::right>(ref<CompareExpr>))  //
             | ref<CompareExpr>> {};

  struct OrExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<OrExpr>) +
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::logical_or>(ref<OR>)) +
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

  struct Query
      : rule<assign<&ast::Query::expression>(ref<Expression>)> {};

  using entry = Query;
  using ignored = WS;
  using hidden = list<SL_COMMENT>;
};

}  // namespace promql
