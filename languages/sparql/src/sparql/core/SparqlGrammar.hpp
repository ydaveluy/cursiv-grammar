// The sparql language: its grammar. SPARQL 1.0 -- a prologue, one of
// SELECT/CONSTRUCT/DESCRIBE/ASK, a graph pattern of triples, OPTIONAL/
// UNION/GRAPH/FILTER, and a solution modifier.
#pragma once

#include <charconv>
#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "sparql/core/ast.hpp"

namespace sparql {

inline std::string_view decode(std::string_view text,
                               std::pmr::memory_resource *memory,
                               bool strings = false) {
  if (text.find('\\') == std::string_view::npos)
    return text;
  const auto room = cursiv::text_room(memory, text.size());
  std::size_t size = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '\\' || i + 1 == text.size()) {
      room[size++] = text[i];
      continue;
    }
    const char escaped = text[++i];
    if (escaped == 'u' || escaped == 'U') {
      const std::size_t count = escaped == 'u' ? 4 : 8;
      std::uint32_t cp = 0;
      std::from_chars(text.data() + i + 1, text.data() + i + 1 + count, cp, 16);
      i += count;
      if (cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
        cp = 0xfffd;
      if (cp < 0x80)
        room[size++] = static_cast<char>(cp);
      else {
        if (cp >= 0x10000)
          room[size++] = static_cast<char>(0xf0 | (cp >> 18));
        else if (cp >= 0x800)
          room[size++] = static_cast<char>(0xe0 | (cp >> 12));
        else
          room[size++] = static_cast<char>(0xc0 | (cp >> 6));
        if (cp >= 0x10000)
          room[size++] = static_cast<char>(0x80 | ((cp >> 12) & 63));
        if (cp >= 0x800)
          room[size++] = static_cast<char>(0x80 | ((cp >> 6) & 63));
        room[size++] = static_cast<char>(0x80 | (cp & 63));
      }
    } else if (!strings) {
      room[size++] = '\\';
      room[size++] = escaped;
    } else
      switch (escaped) {
      case 't':
        room[size++] = '\t';
        break;
      case 'b':
        room[size++] = '\b';
        break;
      case 'n':
        room[size++] = '\n';
        break;
      case 'r':
        room[size++] = '\r';
        break;
      case 'f':
        room[size++] = '\f';
        break;
      default:
        room[size++] = escaped;
        break;
      }
  }
  return {room.data(), size};
}
inline constexpr auto unicode_text = [](std::string_view text,
                                        std::pmr::memory_resource *memory) {
  return decode(text, memory);
};
inline constexpr auto prefix_text = [](std::string_view text,
                                       std::pmr::memory_resource *memory) {
  return decode(text.substr(0, text.size() - 1), memory);
};
inline constexpr auto head_text = [](std::string_view text,
                                     std::pmr::memory_resource *memory) {
  return decode(text.substr(1), memory);
};
inline constexpr auto iri_text = [](std::string_view text,
                                    std::pmr::memory_resource *memory) {
  return decode(text.substr(1, text.size() - 2), memory);
};
inline constexpr auto string_text = [](std::string_view text,
                                       std::pmr::memory_resource *memory) {
  const std::size_t quotes =
      text.size() >= 6 && text[0] == text[1] && text[1] == text[2] ? 3 : 1;
  return decode(text.substr(quotes, text.size() - 2 * quotes), memory, true);
};

struct Sparql : cursiv::GrammarSpec {
  struct LINE_COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
  struct WS : terminal<some(cr<R"( \t\n\r)">)> {};

  // `a` (rdf:type) alone stays case-sensitive; every other keyword below
  // is `ikw<>`.
  struct A : terminal<kw<"a">> {};

  static constexpr auto hex = cr<"0-9a-fA-F">;
  static constexpr auto uchar =
      (lit<"\\u"> + repeat<4>(hex)) | (lit<"\\U"> + repeat<8>(hex));
  static constexpr auto pn_chars_base =
      cr<"A-Za-zÀ-ÖØ-öø-˿Ͱ-ͽͿ-῿‌-‍⁰-↏Ⰰ-⿯、-퟿豈-﷏ﷰ-�\U00010000-"
         "\U000EFFFF">;
  static constexpr auto pn_chars_u = pn_chars_base | cr<"_">;
  static constexpr auto pn_chars = pn_chars_u | cr<"0-9·̀-ͯ‿-⁀-">;
  static constexpr auto pn_prefix =
      (pn_chars_base | uchar) +
      option(many(pn_chars | uchar | lit<".">) + (pn_chars | uchar));
  static constexpr auto pname_ns_body = option(pn_prefix) + lit<":">;
  static constexpr auto plx =
      (lit<"%"> + repeat<2>(hex)) | (lit<"\\"> + cr<"_~.!$&'()*+,;=/?#@%-">);
  static constexpr auto pn_local =
      (pn_chars_u | uchar | cr<"0-9:"> | plx) +
      option(many(pn_chars | uchar | cr<".:"> | plx) +
             (pn_chars | uchar | lit<":"> | plx));
  struct PNAME_NS : terminal<pname_ns_body> {};
  struct PNAME_LN : terminal<pname_ns_body + pn_local> {};

  static constexpr auto blank_node_label =
      lit<"_:"> +
      (pn_chars_u | cr<"0-9">)+option(many(pn_chars | lit<".">) + pn_chars);
  inline static constexpr auto blank_label = [](std::string_view text) {
    return text.substr(2);
  };
  struct BLANK_NODE_LABEL : terminal<as<blank_label>(blank_node_label)> {};
  static constexpr auto varname_extra = cr<"·̀-ͯ‿-⁀">;
  static constexpr auto varname =
      (pn_chars_u | cr<"0-9"> | uchar) +
      many(pn_chars_u | cr<"0-9"> | varname_extra | uchar);
  struct VAR1 : terminal<as<head_text>(lit<"?"> + varname)> {};
  struct VAR2 : terminal<as<head_text>(lit<"$"> + varname)> {};

  struct LANGTAG : terminal<as<head_text>(
                       lit<"@"> + some(cr<"a-zA-Z">) +
                       many(lit<"-"> + some(cr<"a-zA-Z0-9">)))> {};

  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto decimal =
      (some(d) + lit<"."> + many(d)) | (lit<"."> + some(d));
  static constexpr auto double_ = (some(d) + lit<"."> + many(d) + exponent) |
                                  (option(lit<".">) + some(d) + exponent);
  struct INTEGER : terminal<some(d)> {};
  struct NUMERIC_LITERAL
      : terminal<option(cr<"+-">) + (double_ | decimal | some(d))> {};

  // `ECHAR ::= '\' [tbnrf"'\]`: every escape a string literal admits,
  // backslash included.
  static constexpr auto echar = lit<"\\"> + cr<R"(tbnrf"'\\)">;

  struct STRING_LITERAL1
      : terminal<as<string_text>(
            lit<"'"> + many(echar | uchar | cr<R"(^'\\\n\r)">) + lit<"'">)> {};
  struct STRING_LITERAL2
      : terminal<as<string_text>(
            lit<"\""> + many(echar | uchar | cr<R"(^"\\\n\r)">) + lit<"\"">)> {
  };

  // A triple-quoted string excludes its own quote character and `\`; a
  // raw run of the OTHER quote character is unrestricted.
  struct STRING_LITERAL_LONG1
      : terminal<as<string_text>(lit<"'''"> +
                                 many(option(lit<"''"> | lit<"'">) +
                                      (echar | uchar | cr<R"(^'\\)">)) +
                                 lit<"'''">)> {};
  struct STRING_LITERAL_LONG2
      : terminal<as<string_text>(lit<"\"\"\""> +
                                 many(option(lit<"\"\""> | lit<"\"">) +
                                      (echar | uchar | cr<R"(^"\\)">)) +
                                 lit<"\"\"\"">)> {};

  struct IRI_REF
      : terminal<as<iri_text>(lit<"<"> + many(uchar | cr<R"(^\x00- <>"{}|^`\\)">) +
                              lit<">">)> {};

  // Only PN_CHARS may abut a keyword: '.', ':', '?' and '$' stay outside so
  // a term like `ex:a` or a variable's own sigil still lexes as intended.
  static constexpr auto word_boundary = pn_chars;

  // Operators, each denoting its own constant -- `op` holds what a step
  // means, never what it spelled.

  static constexpr auto OpMul = as<ast::Operator::multiply>(lit<"*">) | as<ast::Operator::divide>(lit<"/">);
  static constexpr auto OpAdd = as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">);
  static constexpr auto OpCompare = as<ast::Operator::not_equal>(lit<"!=">) | as<ast::Operator::less_equal>(lit<"<=">) |
                                    as<ast::Operator::greater_equal>(lit<">=">) | as<ast::Operator::equal>(lit<"=">) |
                                    as<ast::Operator::less>(lit<"<">) | as<ast::Operator::greater>(lit<">">);
  static constexpr auto OpAnd = as<ast::Operator::logical_and>(lit<"&&">);
  static constexpr auto OpOr = as<ast::Operator::logical_or>(lit<"||">);

  // BuiltInCall's function keywords, each denoting its own `BuiltIn` tag.
  // isURI and isIRI are the one predicate under two spellings (SPARQL
  // synonyms): both denote `is_iri`.

  struct IriRef;
  struct PrefixedTerm;
  struct BlankNode;
  struct RdfType;
  struct NilTerm;
  struct Var;
  struct NumberLitRule;
  struct Projection;
  struct BoolLit;
  struct StringLit;
  struct Collection;
  struct BlankNodePropertyList;
  struct PredicateObject;
  struct PredicateObjectList;
  struct Triples;
  struct Unary;
  struct PrimaryExpr;
  struct NumericExpr;
  struct Relational;
  struct Expression;
  struct BuiltInCallRule;
  struct FunctionCall;
  struct IriRefOrFunction;
  struct Filter;
  struct GroupGraphPattern;
  struct OptionalGraphPattern;
  struct GraphGraphPattern;
  struct GroupOrUnionGraphPattern;
  struct DatasetClause;
  struct OrderCondition;
  struct SolutionModifier;
  struct PrefixDecl;
  struct Prologue;
  struct SelectQuery;
  struct ConstructQuery;
  struct DescribeQuery;
  struct AskQuery;
  struct Query;

  static constexpr auto IriTerm = ref<IriRef> | ref<PrefixedTerm>;
  static constexpr auto GraphTerm = IriTerm | ref<StringLit> |
                                    ref<NumberLitRule> | ref<BoolLit> |
                                    ref<BlankNode> | ref<NilTerm>;
  static constexpr auto VarOrTerm = ref<Var> | GraphTerm;
  static constexpr auto VarOrIriRef = ref<Var> | IriTerm;
  static constexpr auto TriplesNode =
      ref<Collection> | ref<BlankNodePropertyList>;
  static constexpr auto GraphNode = VarOrTerm | TriplesNode;
  static constexpr auto Verb = VarOrIriRef | ref<RdfType>;
  static constexpr auto Bracketed = lit<"("> + ref<Expression> + lit<")">;
  static constexpr auto Constraint =
      Bracketed | ref<BuiltInCallRule> | ref<FunctionCall>;

  struct IriRef : rule<assign<&ast::IriRef::value>(ref<IRI_REF>)> {};
  struct PrefixName : rule<as<prefix_text>(ref<PNAME_NS>)> {};
  struct PrefixedValue : rule<as<unicode_text>(ref<PNAME_LN> | ref<PNAME_NS>)> {
  };
  struct PrefixedTerm
      : rule<assign<&ast::PrefixedTerm::value>(ref<PrefixedValue>)> {};
  struct BlankNode
      : rule<assign<&ast::BlankNode::label>(ref<BLANK_NODE_LABEL>) |
             (create<ast::BlankNode> + lit<"["> + lit<"]">)> {};
  struct RdfType : rule<create<ast::RdfType> + ref<A>> {};
  struct NilTerm : rule<create<ast::NilTerm> + lit<"("> + lit<")">> {};

  struct Var : rule<assign<&ast::Var::name>(ref<VAR1> | ref<VAR2>)> {};

  struct NumberLitRule
      : rule<assign<&ast::NumberLit::text>(ref<NUMERIC_LITERAL>)> {};

  struct BoolLit
      : rule<assign<&ast::BoolLit::value>(ikw<"true"> | ikw<"false">)> {};

  struct StringLit
      : rule<assign<&ast::StringLit::value>(
                 ref<STRING_LITERAL_LONG1> | ref<STRING_LITERAL_LONG2> |
                 ref<STRING_LITERAL1> | ref<STRING_LITERAL2>) +
             option(assign<&ast::StringLit::lang>(ref<LANGTAG>) |
                    (lit<"^^"> + assign<&ast::StringLit::datatype>(IriTerm)))> {
  };

  struct Collection
      : rule<lit<"("> + some(append<&ast::Collection::items>(GraphNode)) +
             lit<")">> {};

  struct PredicateObject
      : rule<create<ast::PredicateObject>(
            assign<&ast::PredicateObject::predicate>(Verb) +
            some(append<&ast::PredicateObject::objects>(GraphNode),
                 lit<",">))> {};

  struct PredicateObjectList
      : rule<create<ast::PredicateObjectList>(
            append<&ast::PredicateObjectList::items>(ref<PredicateObject>) +
            many(lit<";"> + option(append<&ast::PredicateObjectList::items>(
                                ref<PredicateObject>))))> {};

  struct BlankNodePropertyList
      : rule<lit<"["> +
             assign<&ast::BlankNodePropertyList::properties>(
                 ref<PredicateObjectList>) +
             lit<"]">> {};

  struct Triples
      : rule<create<ast::Triple>(
            (assign<&ast::Triple::subject>(VarOrTerm) +
             assign<&ast::Triple::properties>(ref<PredicateObjectList>)) |
            (assign<&ast::Triple::subject>(TriplesNode) +
             option(assign<&ast::Triple::properties>(
                 ref<PredicateObjectList>))))> {};

  // A `!`/`+`/`-` prefix directly on a digit run is unreachable: the
  // signed numeric terminals below win by length wherever both are
  // offered, so `-5` is always a signed NumberLit, never `UnaryExpr(-,
  // NumberLit(5))`.
  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"!"> | lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<PrimaryExpr>)) |
             ref<PrimaryExpr>> {};

  struct NumericExpr
      : infix<Unary, ast::BinaryExpr, levels(left(OpMul), left(OpAdd))> {};

  // One optional comparison, never a chain: `a < b < c` is not SPARQL, so
  // this sits outside the infix ladders rather than repeating like
  // `*`/`+`/`&&`/`||` do.
  struct Relational
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<NumericExpr>) +
                 assign<&ast::BinaryExpr::op>(OpCompare) +
                 assign<&ast::BinaryExpr::right>(ref<NumericExpr>)) |
             ref<NumericExpr>> {};

  struct Expression
      : infix<Relational, ast::BinaryExpr, levels(left(OpAnd), left(OpOr))> {};

  static constexpr auto BuiltInName =
      as<ast::BuiltIn::str>(ikw<"STR">) | as<ast::BuiltIn::lang>(ikw<"LANG">) | as<ast::BuiltIn::lang_matches>(ikw<"LANGMATCHES">) | as<ast::BuiltIn::datatype>(ikw<"DATATYPE">) |
      as<ast::BuiltIn::bound>(ikw<"BOUND">) | as<ast::BuiltIn::same_term>(ikw<"sameTerm">) | as<ast::BuiltIn::is_iri>(ikw<"isIRI">) | as<ast::BuiltIn::is_iri>(ikw<"isURI">) |
      as<ast::BuiltIn::is_blank>(ikw<"isBLANK">) | as<ast::BuiltIn::is_literal>(ikw<"isLITERAL">) | as<ast::BuiltIn::regex>(ikw<"REGEX">);
  struct BuiltInCallRule
      : rule<assign<&ast::BuiltInCall::fn>(BuiltInName) + lit<"("> +
             many(append<&ast::BuiltInCall::args>(ref<Expression>),
                  lit<",">) +
             lit<")">> {};
  static constexpr auto ArgListInto =
      lit<"("> +
      many(append<&ast::FuncCall::args>(ref<Expression>), lit<",">) +
      lit<")">;

  struct FunctionCall
      : rule<create<ast::FuncCall>(assign<&ast::FuncCall::target>(IriTerm) +
                                   ArgListInto)> {};

  struct IriRefOrFunction
      : rule<create<ast::FuncCall>(assign<&ast::FuncCall::target>(IriTerm) +
                                   ArgListInto) |
             IriTerm> {};

  struct PrimaryExpr
      : rule<Bracketed | ref<BuiltInCallRule> | ref<IriRefOrFunction> |
             ref<StringLit> | ref<NumberLitRule> | ref<BoolLit> |
             ref<Var>> {};

  struct Filter
      : rule<create<ast::Filter>(
            ikw<"FILTER"> + assign<&ast::Filter::constraint>(Constraint))> {};

  struct OptionalGraphPattern
      : rule<create<ast::OptionalGraphPattern>(
            ikw<"OPTIONAL"> + assign<&ast::OptionalGraphPattern::pattern>(
                                  ref<GroupGraphPattern>))> {};

  struct GraphGraphPattern
      : rule<create<ast::GraphGraphPattern>(
            ikw<"GRAPH"> + assign<&ast::GraphGraphPattern::graph>(VarOrIriRef) +
            assign<&ast::GraphGraphPattern::pattern>(
                ref<GroupGraphPattern>))> {};

  struct GroupOrUnionGraphPattern
      : rule<create<ast::GroupOrUnionGraphPattern>(
            append<&ast::GroupOrUnionGraphPattern::alternatives>(
                ref<GroupGraphPattern>) +
            many(ikw<"UNION"> +
                 append<&ast::GroupOrUnionGraphPattern::alternatives>(
                     ref<GroupGraphPattern>)))> {};

  static constexpr auto TriplesInto =
      some(append<&ast::GroupGraphPattern::elements>(ref<Triples>),
           lit<".">) +
      option(lit<".">);

  struct GroupGraphPattern
      : rule<create<ast::GroupGraphPattern>(
            lit<"{"> + option(TriplesInto) +
            many((append<&ast::GroupGraphPattern::elements>(
                      ref<OptionalGraphPattern>) |
                  append<&ast::GroupGraphPattern::elements>(
                      ref<GroupOrUnionGraphPattern>) |
                  append<&ast::GroupGraphPattern::elements>(
                      ref<GraphGraphPattern>) |
                  append<&ast::GroupGraphPattern::elements>(ref<Filter>)) +
                 option(lit<".">) + option(TriplesInto)) +
            lit<"}">)> {};

  struct DatasetClause
      : rule<create<ast::DatasetClause>(
            ikw<"FROM"> +
            option(flag<&ast::DatasetClause::named>(ikw<"NAMED">)) +
            assign<&ast::DatasetClause::graph>(IriTerm))> {};

  struct OrderCondition
      : rule<create<ast::OrderCondition>(
            ((flag<&ast::OrderCondition::descending>(ikw<"DESC">) |
              ikw<"ASC">)+assign<&ast::OrderCondition::value>(Bracketed)) |
            assign<&ast::OrderCondition::value>(Constraint) |
            assign<&ast::OrderCondition::value>(ref<Var>))> {};

  struct SolutionModifier
      : rule<create<ast::SolutionModifier>(
            option(ikw<"ORDER"> + ikw<"BY"> +
                   some(append<&ast::SolutionModifier::orderBy>(
                       ref<OrderCondition>))) +
            option(
                (ikw<"LIMIT"> +
                 assign<&ast::SolutionModifier::limit>(ref<INTEGER>) +
                 option(ikw<"OFFSET"> +
                        assign<&ast::SolutionModifier::offset>(ref<INTEGER>))) |
                (ikw<"OFFSET"> +
                 assign<&ast::SolutionModifier::offset>(ref<INTEGER>) +
                 option(ikw<"LIMIT"> + assign<&ast::SolutionModifier::limit>(
                                           ref<INTEGER>)))))> {};

  struct PrefixDecl
      : rule<create<ast::PrefixDecl>(
            ikw<"PREFIX"> +
            assign<&ast::PrefixDecl::prefix>(ref<PrefixName>) +
            assign<&ast::PrefixDecl::iri>(ref<IRI_REF>))> {};

  struct Prologue
      : rule<create<ast::Prologue>(
            option(ikw<"BASE"> +
                   assign<&ast::Prologue::base>(ref<IriRef>)) +
            many(append<&ast::Prologue::prefixes>(ref<PrefixDecl>)))> {};

  struct Projection
      : rule<lit<"("> +
             assign<&ast::Projection::expression>(ref<Expression>) +
             ikw<"AS"> + assign<&ast::Projection::variable>(ref<Var>) +
             lit<")">> {};

  struct SelectQuery
      : rule<create<ast::SelectQuery>(
            ikw<"SELECT"> +
            option(flag<&ast::SelectQuery::distinct>(ikw<"DISTINCT">) |
                   flag<&ast::SelectQuery::reduced>(ikw<"REDUCED">)) +
            (flag<&ast::SelectQuery::star>(lit<"*">) |
             some(append<&ast::SelectQuery::vars>(ref<Var> |
                                                  ref<Projection>))) +
            many(append<&ast::SelectQuery::datasets>(ref<DatasetClause>)) +
            option(ikw<"WHERE">) +
            assign<&ast::SelectQuery::where>(ref<GroupGraphPattern>) +
            assign<&ast::SelectQuery::modifier>(ref<SolutionModifier>))> {};

  static constexpr auto ConstructTriplesInto =
      some(append<&ast::ConstructQuery::templateTriples>(ref<Triples>),
           lit<".">) +
      option(lit<".">);

  struct ConstructQuery
      : rule<create<ast::ConstructQuery>(
            ikw<"CONSTRUCT"> + lit<"{"> + option(ConstructTriplesInto) +
            lit<"}"> +
            many(append<&ast::ConstructQuery::datasets>(
                ref<DatasetClause>)) +
            option(ikw<"WHERE">) +
            assign<&ast::ConstructQuery::where>(ref<GroupGraphPattern>) +
            assign<&ast::ConstructQuery::modifier>(
                ref<SolutionModifier>))> {};

  struct DescribeQuery
      : rule<create<ast::DescribeQuery>(
            ikw<"DESCRIBE"> +
            (flag<&ast::DescribeQuery::star>(lit<"*">) |
             some(append<&ast::DescribeQuery::vars>(VarOrIriRef))) +
            many(
                append<&ast::DescribeQuery::datasets>(ref<DatasetClause>)) +
            option(option(ikw<"WHERE">) + assign<&ast::DescribeQuery::where>(
                                              ref<GroupGraphPattern>)) +
            assign<&ast::DescribeQuery::modifier>(ref<SolutionModifier>))> {
  };

  struct AskQuery
      : rule<create<ast::AskQuery>(
            ikw<"ASK"> +
            many(append<&ast::AskQuery::datasets>(ref<DatasetClause>)) +
            option(ikw<"WHERE">) +
            assign<&ast::AskQuery::where>(ref<GroupGraphPattern>))> {};

  struct Query : rule<create<ast::Query>(
                         assign<&ast::Query::prologue>(ref<Prologue>) +
                         assign<&ast::Query::form>(
                             ref<SelectQuery> | ref<ConstructQuery> |
                             ref<DescribeQuery> | ref<AskQuery>))> {};

  using entry = Query;
  using ignored = list<WS, LINE_COMMENT>;
};

} // namespace sparql
