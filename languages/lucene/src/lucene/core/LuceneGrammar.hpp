#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "lucene/core/ast.hpp"
namespace lucene {
struct Lucene : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\n\r\u3000)">)> {};
  using ignore = WS;
  static constexpr auto Escaped = lit<"\\"> + any;
  static constexpr auto TermCharacter =
      cr<R"(^ \t\n\r\u3000+!():^[]"{}~*?\\/-)">;
  static constexpr auto TermStart = TermCharacter | Escaped;
  static constexpr auto TermPart = TermStart | cr<"+-">;
  static constexpr auto word_boundary = cr<R"(^ \t\n\r\u3000!():^[]"{}~/)">;
  struct FIELD : terminal<(TermStart + many(TermPart)) | lit<"*">> {};
  struct WORD : terminal<((TermStart | cr<"*?">)+many(TermPart | cr<"*?">)) |
                         (cr<"+!-"> + &ref<WS>)> {};
  struct QUOTED
      : terminal<lit<"\""> + many(cr<R"(^"\\)"> | Escaped) + lit<"\"">> {};
  struct REGEX
      : terminal<lit<"/"> + many(lit<"\\/"> | cr<"^/">) + lit<"/">> {};
  struct BOOST : terminal<lit<"^"> + some(d) + option(lit<"."> + some(d))> {};
  struct FUZZY : terminal<lit<"~"> + many(TermPart)> {};
  static constexpr auto RangeChar = cr<"^ ]}">;
  struct RANGE_GOOP : terminal<some(RangeChar)> {};
  struct RANGE_QUOTED
      : terminal<lit<"\""> + some(lit<"\\\""> | cr<R"(^")">) + lit<"\"">> {
  };
  struct AND
      : terminal<kw<"AND"> | lit<"&&">, value<ast::Conjunction::logical_and>> {
  };
  struct OR
      : terminal<kw<"OR"> | lit<"||">, value<ast::Conjunction::logical_or>> {};
  struct PLUS : terminal<lit<"+", !ref<WS>>, value<ast::Modifier::require>> {};
  struct MINUS : terminal<lit<"-", !ref<WS>>, value<ast::Modifier::prohibit>> {
  };
  struct NOT
      : terminal<kw<"NOT"> | lit<"!", !ref<WS>>, value<ast::Modifier::negate>> {
  };

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
  struct Query;
  struct Group : rule<lit<"("> + assign<&ast::GroupingExpr::query>(ref<Query>) +
                      lit<")"> +
                      many(append<&ast::GroupingExpr::suffixes>(ref<Suffix>))> {
  };
  struct Clause
      : rule<option(assign<&ast::Clause::conjunction>(ref<AND> | ref<OR>)) +
             option(assign<&ast::Clause::modifier>(ref<PLUS> | ref<MINUS> |
                                                   ref<NOT>)) +
             option(assign<&ast::Clause::field>(ref<FIELD>) + lit<":">) +
             assign<&ast::Clause::value>(ref<Group> | ref<Term>)> {};
  struct Query : rule<create<ast::Query> +
                      many(append<&ast::Query::clauses>(ref<Clause>))> {};
  using entry = Query;
};
} // namespace lucene
