// The cql language: Z39.50 Contextual Query Language.
#pragma once

#include <memory_resource>
#include <string_view>

#include "cql/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace cql {

inline constexpr auto unquote = [](std::string_view t,
                                   std::pmr::memory_resource*) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Cql : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  // The identifier alphabet includes the dot, so `dc.title` lexes as one
  // token; the keyword boundary is widened to match, or `AND.` would split.
  static constexpr auto word_boundary = cr<R"(^ \t\r\n()=<>/")">;

  struct AND : terminal<ikw<"AND">> {};
  struct OR : terminal<ikw<"OR">> {};
  struct NOT : terminal<ikw<"NOT">> {};
  struct PROX : terminal<ikw<"PROX">> {};
  struct SORTBY : terminal<ikw<"SORTBY">> {};

  struct CHARSTRING1 : terminal<some(cr<R"(^ \t\r\n()=<>/")">)> {};
  struct CHARSTRING2
      : terminal<through(lit<"\"">, lit<"\"">), Unquoted> {};

  struct COMPARITOR_SYMBOL
      : terminal<lit<">="> | lit<"<="> | lit<"<>"> | lit<"=="> | lit<"="> |
                 lit<">"> | lit<"<">> {};

  struct Identifier : rule<ref<CHARSTRING1> | ref<CHARSTRING2>> {};

  // A single terminal covers every position that takes a prefix, a URI, a
  // modifier name or value, a search term, or an index: an identifier, or
  // one of the reserved words read back as a plain word.
  struct Term
      : rule<ref<Identifier> | ref<AND> | ref<OR> | ref<NOT> | ref<PROX> |
             ref<SORTBY>> {};

  struct Comparitor : rule<ref<COMPARITOR_SYMBOL> | ref<Identifier>> {};

  struct Modifier
      : rule<create<ast::Modifier>(
            lit<"/"> + assign<&ast::Modifier::name>(ref<Term>) +
            option(assign<&ast::Modifier::comparitor>(ref<COMPARITOR_SYMBOL>) +
                   assign<&ast::Modifier::value>(ref<Term>)))> {};

  struct Relation
      : rule<create<ast::Relation>(
            assign<&ast::Relation::comparitor>(ref<Comparitor>) +
            many(append<&ast::Relation::modifiers>(ref<Modifier>)))> {};

  struct BooleanGroup
      : rule<create<ast::BooleanGroup>(
            assign<&ast::BooleanGroup::op>(ref<AND> | ref<OR> | ref<NOT> |
                                           ref<PROX>) +
            many(append<&ast::BooleanGroup::modifiers>(ref<Modifier>)))> {};

  struct PrefixAssignment
      : rule<create<ast::PrefixAssignment>(
            lit<">"> +
            ((assign<&ast::PrefixAssignment::prefix>(ref<Term>) + lit<"="> +
              assign<&ast::PrefixAssignment::uri>(ref<Term>)) |
             assign<&ast::PrefixAssignment::uri>(ref<Term>)))> {};

  struct SortIndex
      : rule<create<ast::SortIndex>(
            assign<&ast::SortIndex::index>(ref<Term>) +
            many(append<&ast::SortIndex::modifiers>(ref<Modifier>)))> {};

  struct ScopedClause;
  struct CqlQueryRule;

  struct SearchClause
      : rule<create<ast::GroupClause>(
                 lit<"("> + assign<&ast::GroupClause::query>(ref<CqlQueryRule>) +
                 lit<")">) |
             create<ast::SearchClause>(
                 assign<&ast::SearchClause::index>(ref<Term>) +
                 assign<&ast::SearchClause::relation>(ref<Relation>) +
                 assign<&ast::SearchClause::term>(ref<Term>)) |
             create<ast::SearchClause>(
                 assign<&ast::SearchClause::term>(ref<Term>))> {};

  // AND/OR/NOT/PROX chain at one level, left to right: CQL defines no
  // precedence among them.
  struct ScopedClause
      : rule<create<ast::BooleanExpr>(
                 assign<&ast::BooleanExpr::left>(ref<ScopedClause>) +
                 assign<&ast::BooleanExpr::group>(ref<BooleanGroup>) +
                 assign<&ast::BooleanExpr::right>(ref<SearchClause>)) |
             ref<SearchClause>> {};

  struct CqlQueryRule
      : rule<create<ast::CqlQuery>(
            many(append<&ast::CqlQuery::prefixes>(ref<PrefixAssignment>)) +
            assign<&ast::CqlQuery::clause>(ref<ScopedClause>))> {};

  struct SortedQuery
      : rule<create<ast::Cql>(
            many(append<&ast::Cql::prefixes>(ref<PrefixAssignment>)) +
            assign<&ast::Cql::clause>(ref<ScopedClause>) +
            option(ref<SORTBY> +
                   some(append<&ast::Cql::sortSpec>(ref<SortIndex>))))> {};

  using entry = SortedQuery;
  using ignore = WS;
};

}  // namespace cql
