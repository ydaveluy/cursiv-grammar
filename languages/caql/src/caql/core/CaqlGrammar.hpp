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
    [](std::string_view t, std::pmr::memory_resource* mem)
        -> cursiv::ConversionResult<std::string_view> {
  using Result = cursiv::ConversionResult<std::string_view>;
  const bool percent = t.front() == 'p';
  const auto start = percent ? 2u : 1u;
  const std::string_view body = t.substr(start, t.size() - start - 1);
  if (body.find(percent ? '%' : '\\') == std::string_view::npos)
    return Result::ok(body);
  const std::span<char> room = cursiv::text_room(mem, body.size());
  const auto hex = [](char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    char c = body[i];
    if (percent && c == '%') {
      if (i + 2 >= body.size() || hex(body[i + 1]) < 0 ||
          hex(body[i + 2]) < 0)
        return Result::err("Expected two hex digits after % in a CAQL string.");
      c = static_cast<char>(hex(body[i + 1]) * 16 + hex(body[i + 2]));
      i += 2;
    } else if (!percent && c == '\\' && i + 1 < body.size()) {
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
  return Result::ok(std::string_view(room.data(), n));
};

struct Caql : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"(\r\t\n )">)> {};

  static constexpr auto word_boundary = cr<"a-zA-Z0-9_:">;

  struct NAME : terminal<cr<"a-zA-Z_:"> + many(cr<"a-zA-Z0-9_:">)> {};

  static constexpr auto NumeralBody = some(d) + option(lit<"."> + some(d));
  struct NUMBER
      : terminal<as<double>(NumeralBody + option(lit<"e"> + option(cr<"-+">) +
                                                 NumeralBody))> {};

  struct STRING
      : terminal<as<unescape_string>(
            option(lit<"p">) +
            ((lit<"'"> + many((lit<"\\"> + any) | cr<R"(^'\\)">) + lit<"'">) |
            (lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
             lit<"\"">)))> {};

  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  struct UUID
      : terminal<repeat<8>(HexDigit) + lit<"-"> + repeat<4>(HexDigit) +
                     lit<"-"> + repeat<4>(HexDigit) + lit<"-"> +
                     repeat<4>(HexDigit) + lit<"-"> + repeat<12>(HexDigit)> {
  };

  static constexpr auto DurationUnit =
      lit<"seconds"> | lit<"second"> | lit<"sec"> | lit<"s"> |
      lit<"minutes"> | lit<"minute"> | lit<"min"> | cr<"mM"> |
      lit<"hours"> | lit<"hour"> | lit<"hr"> | lit<"h"> |
      lit<"days"> | lit<"day"> | lit<"d"> |
      lit<"weeks"> | lit<"week"> | lit<"wk"> | lit<"w">;
  static constexpr auto DurationPart =
      option(cr<"+-">) + NumeralBody + DurationUnit;
  struct DURATION
      : terminal<DurationPart +
                 many(many(cr<R"( \t)">) + DurationPart)> {};
  struct CONSTANT : terminal<kw<"VIEW_PERIOD"> | kw<"VIEW_RANGE">> {};
  struct DIRECTIVE
      : terminal<lit<"#"> + some(cr<R"(^\r\n)">) + (eol | eof)> {};



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
  struct NOT : terminal<kw<"not">> {};

  struct Expression;

  // A soft keyword: every aggregation/join word is also a legal name.
  struct Name
      : rule<ref<NAME> | ref<AND> | ref<OR> | ref<UNLESS> | ref<BY> |
             ref<WITHOUT> | ref<ON> | ref<IGNORING> | ref<GROUP_LEFT> |
             ref<GROUP_RIGHT> | ref<OFFSET> | ref<BOOL> | ref<NOT>> {};

  struct NumberLit
      : rule<assign<&ast::NumberLit::value>(ref<NUMBER>)> {};
  struct StringLit
      : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};
  struct UuidLit
      : rule<assign<&ast::UuidLit::value>(ref<UUID>)> {
  };
  struct DurationLit
      : rule<assign<&ast::DurationLit::text>(ref<DURATION>)> {};
  struct ConstantLit
      : rule<assign<&ast::ConstantLit::name>(ref<CONSTANT>)> {};

  struct Grouping
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

  struct BraceExpr
      : rule<create<ast::BraceExpr>(
            assign<&ast::BraceExpr::name>(ref<Name>) + lit<"{"> +
            some(append<&ast::BraceExpr::operands>(ref<Expression>),
                 lit<",">) +
            lit<"}">)> {};

  struct CallArg
      : rule<create<ast::CallArg>(
            option((flag<&ast::CallArg::limit>(
                        assign<&ast::CallArg::name>(kw<"limit">)) |
                    assign<&ast::CallArg::name>(ref<Name>)) + lit<"=">) +
            assign<&ast::CallArg::value>(ref<Expression>))> {};

  template <auto Args, auto Sources>
  static constexpr auto CallBody =
      (lit<"("> + many(append<Args>(ref<CallArg>), lit<",">) + lit<")"> +
       option(lit<"{"> +
              many(append<Sources>(ref<Expression>), lit<",">) + lit<"}">)) |
      (lit<"{"> + many(append<Sources>(ref<Expression>), lit<",">) + lit<"}">);

  struct CallExpr
      : rule<create<ast::CallExpr>(
            assign<&ast::CallExpr::name>(ref<Name>) +
            CallBody<&ast::CallExpr::args, &ast::CallExpr::sources>)> {};

  struct Atom
      : rule<ref<BraceExpr>  //
             | ref<CallExpr>  //
             | ref<NumberLit>  //
             | ref<DurationLit>
             | ref<ConstantLit>
             | ref<StringLit>  //
             | ref<UuidLit>  //
             | (lit<"("> + ref<Expression> + lit<")">)> {};

  struct HighNot
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(
                     as<ast::Operator::logical_not>(lit<"!">)) +
                 assign<&ast::UnaryExpr::operand>(ref<HighNot>)) |
             ref<Atom>> {};

  // Right-associative, tightest: `a^b^c` is `a^(b^c)`.
  struct PowExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<HighNot>) +
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::pow>(lit<"^">)) +
                 option(
                     assign<&ast::BinaryExpr::matching>(ref<Grouping>)) +
                 assign<&ast::BinaryExpr::right>(ref<PowExpr>))  //
             | ref<HighNot>> {};

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
  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">)) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>))  //
             | ref<Subqueried>> {};

  struct MultExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<MultExpr>) +
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::multiply>(lit<"*">) | as<ast::Operator::divide>(lit<"/">) |
                                              as<ast::Operator::modulo>(lit<"%">)) +
                 option(
                     assign<&ast::BinaryExpr::matching>(ref<Grouping>)) +
                 assign<&ast::BinaryExpr::right>(ref<UnaryExpr>))  //
             | ref<UnaryExpr>> {};

  struct AddExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<AddExpr>) +
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">)) +
                 option(
                     assign<&ast::BinaryExpr::matching>(ref<Grouping>)) +
                 assign<&ast::BinaryExpr::right>(ref<MultExpr>))  //
             | ref<MultExpr>> {};

  struct CompareExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<CompareExpr>) +
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::deq>(lit<"==">) | as<ast::Operator::ne>(lit<"!=">) | as<ast::Operator::gt>(lit<">">) |
                                              as<ast::Operator::lt>(lit<"<">) | as<ast::Operator::ge>(lit<">=">) | as<ast::Operator::le>(lit<"<=">)) +
                 option(flag<&ast::BinaryExpr::boolMod>(ref<BOOL>)) +
                 option(
                     assign<&ast::BinaryExpr::matching>(ref<Grouping>)) +
                 assign<&ast::BinaryExpr::right>(ref<AddExpr>))  //
             | ref<AddExpr>> {};

  struct AndUnlessExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<AndUnlessExpr>) +
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::logical_and>(ref<AND>) | as<ast::Operator::logical_unless>(ref<UNLESS>)) +
                 option(
                     assign<&ast::BinaryExpr::matching>(ref<Grouping>)) +
                 assign<&ast::BinaryExpr::right>(ref<CompareExpr>))  //
             | ref<CompareExpr>> {};

  struct OrExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<OrExpr>) +
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::logical_or>(ref<OR>)) +
                 option(
                     assign<&ast::BinaryExpr::matching>(ref<Grouping>)) +
                 assign<&ast::BinaryExpr::right>(ref<AndUnlessExpr>))  //
             | ref<AndUnlessExpr>> {};

  struct LowNot
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(
                     as<ast::Operator::logical_not>(ref<NOT>)) +
                 assign<&ast::UnaryExpr::operand>(ref<LowNot>)) |
             ref<OrExpr>> {};

  // Loosest: the `| name(args)` metrics-aggregation pipe, chainable.
  struct Expression
      : rule<create<ast::PipeExpr>(
                 assign<&ast::PipeExpr::operand>(ref<Expression>) +
                 lit<"|"> + assign<&ast::PipeExpr::name>(ref<Name>) +
                 CallBody<&ast::PipeExpr::args, &ast::PipeExpr::sources>)  //
             | ref<LowNot>> {};

  struct Query
      : rule<create<ast::Query>(
            many(append<&ast::Query::directives>(ref<DIRECTIVE>)) +
            assign<&ast::Query::expression>(ref<Expression>))> {};

  using entry = Query;
  using ignored = WS;
};

}  // namespace caql
