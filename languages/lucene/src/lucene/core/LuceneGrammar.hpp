#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "lucene/core/ast.hpp"
namespace lucene {
struct Lucene : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\n\r\u3000)">)> {};
  using ignored = WS;
  static constexpr auto Escaped = lit<"\\"> + any;
  static constexpr auto TermCharacter =
      cr<R"(^ \t\n\r\u3000+!():^@<>=[]"{}~\\/-)">;
  static constexpr auto TermStart = TermCharacter | Escaped;
  static constexpr auto TermPart = TermStart | cr<"+-">;
  static constexpr auto decimal = some(d) + option(lit<"."> + some(d));
  struct NUMBER : terminal<decimal + !TermPart> {};
  struct FIELD : terminal<!(decimal + !TermPart) + !lit<"fn:"> +
                          TermStart + many(TermPart)> {};
  struct WORD : terminal<!lit<"fn:"> + TermStart + many(TermPart)> {};
  struct QUOTED
      : terminal<lit<"\""> + many(cr<R"(^"\\)"> | Escaped) + lit<"\"">> {};
  struct REGEX
      : terminal<lit<"/"> + many(lit<"\\/"> | cr<"^/">) + lit<"/">> {};
  struct BOOST : rule<lit<"^"> + ref<NUMBER>> {};
  struct FUZZY : rule<lit<"~"> + option(ref<NUMBER>)> {};
  static constexpr auto RangeChar = cr<"^ ]}">;
  struct RANGE_GOOP : terminal<some(RangeChar)> {};
  struct RANGE_QUOTED
      : terminal<lit<"\""> + some(lit<"\\\""> | cr<R"(^")">) + lit<"\"">> {
  };
  struct AND
      : terminal<as<ast::Conjunction::logical_and>(kw<"AND"> | lit<"&&">)> {};
  struct OR : terminal<as<ast::Conjunction::logical_or>(kw<"OR"> | lit<"||">)> {
  };
  struct NOT
      : terminal<as<ast::Modifier::negate>(kw<"NOT"> | lit<"!">)> {};

  struct Word : rule<assign<&ast::WordTerm::raw>(ref<WORD>)> {};
  struct Quoted : rule<assign<&ast::QuotedTerm::raw>(ref<QUOTED>)> {};
  struct Regex : rule<assign<&ast::RegexTerm::raw>(ref<REGEX>)> {};
  struct Bound : rule<flag<&ast::RangeBound::quoted>(
                          assign<&ast::RangeBound::raw>(ref<RANGE_QUOTED>)) |
                      assign<&ast::RangeBound::raw>(ref<RANGE_GOOP>)> {};
  struct Range
      : rule<(flag<&ast::RangeTerm::leftInclusive>(lit<"[">) |
              lit<"{">)+assign<&ast::RangeTerm::left>(ref<Bound>) +
             lit<"TO", !RangeChar> +
             assign<&ast::RangeTerm::right>(ref<Bound>) +
             (flag<&ast::RangeTerm::rightInclusive>(lit<"]">) | lit<"}">)> {};
  struct Suffix : rule<assign<&ast::Suffix::raw>(ref<BOOST> | ref<FUZZY>)> {};
  struct Term : rule<assign<&ast::Term::primary>(ref<Range> | ref<Quoted> |
                                                 ref<Regex> | ref<Word>) +
                     many(append<&ast::Term::suffixes>(ref<Suffix>))> {};
  // Interval signatures share one structured argument vector. Numeric arguments
  // retain their own WordTerm node; their position follows the named signature.
  struct Interval;
  struct IntervalNumber : rule<assign<&ast::WordTerm::raw>(ref<NUMBER>)> {};
  static constexpr auto intervalArg =
      append<&ast::IntervalExpr::arguments>(ref<Interval> | ref<Quoted> | ref<Word>);
  static constexpr auto intervalText =
      append<&ast::IntervalExpr::arguments>(ref<Quoted> | ref<Word>);
  static constexpr auto intervalNumber =
      append<&ast::IntervalExpr::arguments>(ref<IntervalNumber>);
  struct Interval : rule<create<ast::IntervalExpr>(lit<"fn:"> +
      ((assign<&ast::IntervalExpr::function>(lit<"ordered"> | lit<"unordered"> |
                                            lit<"or"> | lit<"phrase">) +
        lit<"("> + some(intervalArg) + lit<")">) |
       (assign<&ast::IntervalExpr::function>(lit<"atleast"> | lit<"atLeast">) +
        lit<"("> + intervalNumber + some(intervalArg) + lit<")">) |
       (assign<&ast::IntervalExpr::function>(lit<"maxwidth"> | lit<"maxWidth"> |
                                            lit<"maxgaps"> | lit<"maxGaps">) +
        lit<"("> + intervalNumber + intervalArg + lit<")">) |
       (assign<&ast::IntervalExpr::function>(lit<"unorderedNoOverlaps"> |
          lit<"unorderednooverlaps"> | lit<"before"> | lit<"after"> |
          lit<"containing"> | lit<"notContaining"> | lit<"notcontaining"> |
          lit<"containedBy"> | lit<"containedby"> | lit<"notContainedBy"> |
          lit<"notcontainedby"> | lit<"overlapping"> | lit<"nonOverlapping"> |
          lit<"nonoverlapping">) + lit<"("> + intervalArg + intervalArg + lit<")">) |
       (assign<&ast::IntervalExpr::function>(lit<"within"> | lit<"notWithin"> |
                                            lit<"notwithin">) +
        lit<"("> + intervalArg + intervalNumber + intervalArg + lit<")">) |
       (assign<&ast::IntervalExpr::function>(lit<"extend">) +
        lit<"("> + intervalArg + intervalNumber + intervalNumber + lit<")">) |
       (assign<&ast::IntervalExpr::function>(lit<"wildcard">) +
        lit<"("> + intervalText + option(intervalNumber) + lit<")">) |
       (assign<&ast::IntervalExpr::function>(lit<"fuzzyterm"> | lit<"fuzzyTerm">) +
        lit<"("> + intervalText + option(intervalNumber) + option(intervalNumber) + lit<")">)))> {};
  struct Query;
  struct Group : rule<lit<"("> + assign<&ast::GroupingExpr::query>(ref<Query>) +
                      lit<")"> +
                      many(append<&ast::GroupingExpr::suffixes>(ref<Suffix>)) +
                      option(lit<"@"> + assign<&ast::GroupingExpr::minimumShouldMatch>(ref<NUMBER>))> {
  };
  struct Clause
      : rule<option(assign<&ast::Clause::conjunction>(ref<AND> | ref<OR>)) +
             option(assign<&ast::Clause::modifier>(as<ast::Modifier::require>(lit<"+">) | as<ast::Modifier::prohibit>(lit<"-">) |
                                                   ref<NOT>)) +
             ((assign<&ast::Clause::field>(ref<FIELD>) +
               assign<&ast::Clause::fieldOperator>(lit<"<="> | lit<">="> | lit<"<"> | lit<">">) +
               assign<&ast::Clause::value>(ref<Quoted> | ref<Word>)) |
              (option(assign<&ast::Clause::field>(ref<FIELD>) +
                      assign<&ast::Clause::fieldOperator>(lit<":"> | lit<"=">)) +
               assign<&ast::Clause::value>(ref<Interval> | ref<Group> | ref<Term>)))> {};
  struct Query : rule<create<ast::Query> +
                      many(append<&ast::Query::clauses>(ref<Clause>))> {};
  using entry = Query;
};
} // namespace lucene
