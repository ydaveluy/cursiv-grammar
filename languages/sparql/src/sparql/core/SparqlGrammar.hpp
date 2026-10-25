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
using UnicodeText = cursiv::text_conv<unicode_text>;
using PrefixText = cursiv::text_conv<prefix_text>;
using HeadText = cursiv::text_conv<head_text>;
using IriText = cursiv::text_conv<iri_text>;
using StringText = cursiv::text_conv<string_text>;

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
  struct BLANK_NODE_LABEL : terminal<blank_node_label, text_conv<blank_label>> {
  };
  static constexpr auto varname_extra = cr<"·̀-ͯ‿-⁀">;
  static constexpr auto varname =
      (pn_chars_u | cr<"0-9"> | uchar) +
      many(pn_chars_u | cr<"0-9"> | varname_extra | uchar);
  struct VAR1 : terminal<lit<"?"> + varname, HeadText> {};
  struct VAR2 : terminal<lit<"$"> + varname, HeadText> {};

  struct LANGTAG
      : terminal<lit<"@"> + some(pn_chars_base) +
                     many(lit<"-"> + some(pn_chars_base | cr<"0-9">)),
                 HeadText> {};

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
      : terminal<lit<"'"> + many(echar | uchar | cr<R"(^'\\\n\r)">) + lit<"'">,
                 StringText> {};
  struct STRING_LITERAL2
      : terminal<lit<"\""> + many(echar | uchar | cr<R"(^"\\\n\r)">) +
                     lit<"\"">,
                 StringText> {};

  // A triple-quoted string excludes its own quote character and `\`; a
  // raw run of the OTHER quote character is unrestricted.
  struct STRING_LITERAL_LONG1
      : terminal<lit<"'''"> +
                     many(option(lit<"''"> | lit<"'">) +
                          (echar | uchar | cr<R"(^'\\)">)) +
                     lit<"'''">,
                 StringText> {};
  struct STRING_LITERAL_LONG2
      : terminal<lit<"\"\"\""> +
                     many(option(lit<"\"\""> | lit<"\"">) +
                          (echar | uchar | cr<R"(^"\\)">)) +
                     lit<"\"\"\"">,
                 StringText> {};

  struct IRI_REF
      : terminal<lit<"<"> + many(uchar | cr<R"(^<>"{}|^`\\)">) + lit<">">,
                 IriText> {};

  // Only PN_CHARS may abut a keyword: '.', ':', '?' and '$' stay outside so
  // a term like `ex:a` or a variable's own sigil still lexes as intended.
  static constexpr auto word_boundary = pn_chars;

  // Operators, each denoting its own constant -- `op` holds what a step
  // means, never what it spelled.
  struct MULTIPLY : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIVIDE : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct ADD_OP : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct SUB_OP : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct EQUAL_OP : terminal<lit<"=">, value<ast::Operator::equal>> {};
  struct NOT_EQUAL_OP : terminal<lit<"!=">, value<ast::Operator::not_equal>> {};
  struct LESS_EQUAL_OP : terminal<lit<"<=">, value<ast::Operator::less_equal>> {
  };
  struct GREATER_EQUAL_OP
      : terminal<lit<">=">, value<ast::Operator::greater_equal>> {};
  struct LESS_OP : terminal<lit<"<">, value<ast::Operator::less>> {};
  struct GREATER_OP : terminal<lit<">">, value<ast::Operator::greater>> {};
  struct AND_OP : terminal<lit<"&&">, value<ast::Operator::logical_and>> {};
  struct OR_OP : terminal<lit<"||">, value<ast::Operator::logical_or>> {};

  static constexpr auto OpMul = ref<MULTIPLY> | ref<DIVIDE>;
  static constexpr auto OpAdd = ref<ADD_OP> | ref<SUB_OP>;
  static constexpr auto OpCompare = ref<NOT_EQUAL_OP> | ref<LESS_EQUAL_OP> |
                                    ref<GREATER_EQUAL_OP> | ref<EQUAL_OP> |
                                    ref<LESS_OP> | ref<GREATER_OP>;
  static constexpr auto OpAnd = ref<AND_OP>;
  static constexpr auto OpOr = ref<OR_OP>;

  // BuiltInCall's function keywords, each denoting its own `BuiltIn` tag.
  struct STR_KW : terminal<ikw<"STR">, value<ast::BuiltIn::str>> {};
  struct LANG_KW : terminal<ikw<"LANG">, value<ast::BuiltIn::lang>> {};
  struct LANGMATCHES_KW
      : terminal<ikw<"LANGMATCHES">, value<ast::BuiltIn::lang_matches>> {};
  struct DATATYPE_KW
      : terminal<ikw<"DATATYPE">, value<ast::BuiltIn::datatype>> {};
  struct BOUND_KW : terminal<ikw<"BOUND">, value<ast::BuiltIn::bound>> {};
  struct SAME_TERM_KW
      : terminal<ikw<"sameTerm">, value<ast::BuiltIn::same_term>> {};
  // isURI and isIRI are the one predicate under two spellings (SPARQL
  // synonyms): both denote `is_iri`.
  struct IS_IRI_KW : terminal<ikw<"isIRI">, value<ast::BuiltIn::is_iri>> {};
  struct IS_URI_KW : terminal<ikw<"isURI">, value<ast::BuiltIn::is_iri>> {};
  struct IS_BLANK_KW : terminal<ikw<"isBLANK">, value<ast::BuiltIn::is_blank>> {
  };
  struct IS_LITERAL_KW
      : terminal<ikw<"isLITERAL">, value<ast::BuiltIn::is_literal>> {};
  struct REGEX_KW : terminal<ikw<"REGEX">, value<ast::BuiltIn::regex>> {};

  struct IriRefRule;
  struct PrefixedTermRule;
  struct BlankNodeRule;
  struct RdfTypeRule;
  struct NilTermRule;
  struct VarRule;
  struct NumberLitRule;
  struct ProjectionRule;
  struct BoolLitRule;
  struct StringLitRule;
  struct CollectionRule;
  struct BlankNodePropertyListRule;
  struct PredicateObjectRule;
  struct PredicateObjectListRule;
  struct TriplesRule;
  struct UnaryRule;
  struct PrimaryExprRule;
  struct NumericExprRule;
  struct RelationalRule;
  struct ExpressionRule;
  struct BuiltInCallRule;
  struct FunctionCallRule;
  struct IriRefOrFunctionRule;
  struct FilterRule;
  struct GroupGraphPatternRule;
  struct OptionalGraphPatternRule;
  struct GraphGraphPatternRule;
  struct GroupOrUnionGraphPatternRule;
  struct DatasetClauseRule;
  struct OrderConditionRule;
  struct SolutionModifierRule;
  struct PrefixDeclRule;
  struct PrologueRule;
  struct SelectQueryRule;
  struct ConstructQueryRule;
  struct DescribeQueryRule;
  struct AskQueryRule;
  struct QueryRule;

  static constexpr auto IriTerm = ref<IriRefRule> | ref<PrefixedTermRule>;
  static constexpr auto GraphTerm = IriTerm | ref<StringLitRule> |
                                    ref<NumberLitRule> | ref<BoolLitRule> |
                                    ref<BlankNodeRule> | ref<NilTermRule>;
  static constexpr auto VarOrTerm = ref<VarRule> | GraphTerm;
  static constexpr auto VarOrIriRef = ref<VarRule> | IriTerm;
  static constexpr auto TriplesNode =
      ref<CollectionRule> | ref<BlankNodePropertyListRule>;
  static constexpr auto GraphNode = VarOrTerm | TriplesNode;
  static constexpr auto Verb = VarOrIriRef | ref<RdfTypeRule>;
  static constexpr auto Bracketed = lit<"("> + ref<ExpressionRule> + lit<")">;
  static constexpr auto Constraint =
      Bracketed | ref<BuiltInCallRule> | ref<FunctionCallRule>;

  struct IriRefRule : rule<assign<&ast::IriRef::value>(ref<IRI_REF>)> {};
  struct PrefixNameRule : rule<ref<PNAME_NS>, PrefixText> {};
  struct PrefixedValueRule : rule<ref<PNAME_LN> | ref<PNAME_NS>, UnicodeText> {
  };
  struct PrefixedTermRule
      : rule<assign<&ast::PrefixedTerm::value>(ref<PrefixedValueRule>)> {};
  struct BlankNodeRule
      : rule<assign<&ast::BlankNode::label>(ref<BLANK_NODE_LABEL>) |
             (create<ast::BlankNode> + lit<"["> + lit<"]">)> {};
  struct RdfTypeRule : rule<create<ast::RdfType> + ref<A>> {};
  struct NilTermRule : rule<create<ast::NilTerm> + lit<"("> + lit<")">> {};

  struct VarRule : rule<assign<&ast::Var::name>(ref<VAR1> | ref<VAR2>)> {};

  struct NumberLitRule
      : rule<assign<&ast::NumberLit::text>(ref<NUMERIC_LITERAL>)> {};

  struct BoolLitRule
      : rule<assign<&ast::BoolLit::value>(ikw<"true"> | ikw<"false">)> {};

  struct StringLitRule
      : rule<assign<&ast::StringLit::value>(
                 ref<STRING_LITERAL_LONG1> | ref<STRING_LITERAL_LONG2> |
                 ref<STRING_LITERAL1> | ref<STRING_LITERAL2>) +
             option(assign<&ast::StringLit::lang>(ref<LANGTAG>) |
                    (lit<"^^"> + assign<&ast::StringLit::datatype>(IriTerm)))> {
  };

  struct CollectionRule
      : rule<lit<"("> + some(append<&ast::Collection::items>(GraphNode)) +
             lit<")">> {};

  struct PredicateObjectRule
      : rule<create<ast::PredicateObject>(
            assign<&ast::PredicateObject::predicate>(Verb) +
            some(append<&ast::PredicateObject::objects>(GraphNode),
                 lit<",">))> {};

  struct PredicateObjectListRule
      : rule<create<ast::PredicateObjectList>(
            append<&ast::PredicateObjectList::items>(ref<PredicateObjectRule>) +
            many(lit<";"> + option(append<&ast::PredicateObjectList::items>(
                                ref<PredicateObjectRule>))))> {};

  struct BlankNodePropertyListRule
      : rule<lit<"["> +
             assign<&ast::BlankNodePropertyList::properties>(
                 ref<PredicateObjectListRule>) +
             lit<"]">> {};

  struct TriplesRule
      : rule<create<ast::Triple>(
            (assign<&ast::Triple::subject>(VarOrTerm) +
             assign<&ast::Triple::properties>(ref<PredicateObjectListRule>)) |
            (assign<&ast::Triple::subject>(TriplesNode) +
             option(assign<&ast::Triple::properties>(
                 ref<PredicateObjectListRule>))))> {};

  // A `!`/`+`/`-` prefix directly on a digit run is unreachable: the
  // signed numeric terminals below win by length wherever both are
  // offered, so `-5` is always a signed NumberLit, never `UnaryExpr(-,
  // NumberLit(5))`.
  struct UnaryRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"!"> | lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<PrimaryExprRule>)) |
             ref<PrimaryExprRule>> {};

  struct NumericExprRule
      : infix<UnaryRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, (left(OpMul) >> left(OpAdd))> {};

  // One optional comparison, never a chain: `a < b < c` is not SPARQL, so
  // this sits outside the infix ladders rather than repeating like
  // `*`/`+`/`&&`/`||` do.
  struct RelationalRule
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<NumericExprRule>) +
                 assign<&ast::BinaryExpr::op>(OpCompare) +
                 assign<&ast::BinaryExpr::right>(ref<NumericExprRule>)) |
             ref<NumericExprRule>> {};

  struct ExpressionRule
      : infix<RelationalRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, (left(OpAnd) >> left(OpOr))> {};

  static constexpr auto BuiltInName =
      ref<STR_KW> | ref<LANG_KW> | ref<LANGMATCHES_KW> | ref<DATATYPE_KW> |
      ref<BOUND_KW> | ref<SAME_TERM_KW> | ref<IS_IRI_KW> | ref<IS_URI_KW> |
      ref<IS_BLANK_KW> | ref<IS_LITERAL_KW> | ref<REGEX_KW>;
  struct BuiltInCallRule
      : rule<assign<&ast::BuiltInCall::fn>(BuiltInName) + lit<"("> +
             many(append<&ast::BuiltInCall::args>(ref<ExpressionRule>),
                  lit<",">) +
             lit<")">> {};
  static constexpr auto ArgListInto =
      lit<"("> +
      many(append<&ast::FuncCall::args>(ref<ExpressionRule>), lit<",">) +
      lit<")">;

  struct FunctionCallRule
      : rule<create<ast::FuncCall>(assign<&ast::FuncCall::target>(IriTerm) +
                                   ArgListInto)> {};

  struct IriRefOrFunctionRule
      : rule<create<ast::FuncCall>(assign<&ast::FuncCall::target>(IriTerm) +
                                   ArgListInto) |
             IriTerm> {};

  struct PrimaryExprRule
      : rule<Bracketed | ref<BuiltInCallRule> | ref<IriRefOrFunctionRule> |
             ref<StringLitRule> | ref<NumberLitRule> | ref<BoolLitRule> |
             ref<VarRule>> {};

  struct FilterRule
      : rule<create<ast::Filter>(
            ikw<"FILTER"> + assign<&ast::Filter::constraint>(Constraint))> {};

  struct OptionalGraphPatternRule
      : rule<create<ast::OptionalGraphPattern>(
            ikw<"OPTIONAL"> + assign<&ast::OptionalGraphPattern::pattern>(
                                  ref<GroupGraphPatternRule>))> {};

  struct GraphGraphPatternRule
      : rule<create<ast::GraphGraphPattern>(
            ikw<"GRAPH"> + assign<&ast::GraphGraphPattern::graph>(VarOrIriRef) +
            assign<&ast::GraphGraphPattern::pattern>(
                ref<GroupGraphPatternRule>))> {};

  struct GroupOrUnionGraphPatternRule
      : rule<create<ast::GroupOrUnionGraphPattern>(
            append<&ast::GroupOrUnionGraphPattern::alternatives>(
                ref<GroupGraphPatternRule>) +
            many(ikw<"UNION"> +
                 append<&ast::GroupOrUnionGraphPattern::alternatives>(
                     ref<GroupGraphPatternRule>)))> {};

  static constexpr auto TriplesInto =
      some(append<&ast::GroupGraphPattern::elements>(ref<TriplesRule>),
           lit<".">) +
      option(lit<".">);

  struct GroupGraphPatternRule
      : rule<create<ast::GroupGraphPattern>(
            lit<"{"> + option(TriplesInto) +
            many((append<&ast::GroupGraphPattern::elements>(
                      ref<OptionalGraphPatternRule>) |
                  append<&ast::GroupGraphPattern::elements>(
                      ref<GroupOrUnionGraphPatternRule>) |
                  append<&ast::GroupGraphPattern::elements>(
                      ref<GraphGraphPatternRule>) |
                  append<&ast::GroupGraphPattern::elements>(ref<FilterRule>)) +
                 option(lit<".">) + option(TriplesInto)) +
            lit<"}">)> {};

  struct DatasetClauseRule
      : rule<create<ast::DatasetClause>(
            ikw<"FROM"> +
            option(flag<&ast::DatasetClause::named>(ikw<"NAMED">)) +
            assign<&ast::DatasetClause::graph>(IriTerm))> {};

  struct OrderConditionRule
      : rule<create<ast::OrderCondition>(
            ((flag<&ast::OrderCondition::descending>(ikw<"DESC">) |
              ikw<"ASC">)+assign<&ast::OrderCondition::value>(Bracketed)) |
            assign<&ast::OrderCondition::value>(Constraint) |
            assign<&ast::OrderCondition::value>(ref<VarRule>))> {};

  struct SolutionModifierRule
      : rule<create<ast::SolutionModifier>(
            option(ikw<"ORDER"> + ikw<"BY"> +
                   some(append<&ast::SolutionModifier::orderBy>(
                       ref<OrderConditionRule>))) +
            option(
                (ikw<"LIMIT"> +
                 assign<&ast::SolutionModifier::limit>(ref<INTEGER>) +
                 option(ikw<"OFFSET"> +
                        assign<&ast::SolutionModifier::offset>(ref<INTEGER>))) |
                (ikw<"OFFSET"> +
                 assign<&ast::SolutionModifier::offset>(ref<INTEGER>) +
                 option(ikw<"LIMIT"> + assign<&ast::SolutionModifier::limit>(
                                           ref<INTEGER>)))))> {};

  struct PrefixDeclRule
      : rule<create<ast::PrefixDecl>(
            ikw<"PREFIX"> +
            assign<&ast::PrefixDecl::prefix>(ref<PrefixNameRule>) +
            assign<&ast::PrefixDecl::iri>(ref<IRI_REF>))> {};

  struct PrologueRule
      : rule<create<ast::Prologue>(
            option(ikw<"BASE"> +
                   assign<&ast::Prologue::base>(ref<IriRefRule>)) +
            many(append<&ast::Prologue::prefixes>(ref<PrefixDeclRule>)))> {};

  struct ProjectionRule
      : rule<lit<"("> +
             assign<&ast::Projection::expression>(ref<ExpressionRule>) +
             ikw<"AS"> + assign<&ast::Projection::variable>(ref<VarRule>) +
             lit<")">> {};

  struct SelectQueryRule
      : rule<create<ast::SelectQuery>(
            ikw<"SELECT"> +
            option(flag<&ast::SelectQuery::distinct>(ikw<"DISTINCT">) |
                   flag<&ast::SelectQuery::reduced>(ikw<"REDUCED">)) +
            (flag<&ast::SelectQuery::star>(lit<"*">) |
             some(append<&ast::SelectQuery::vars>(ref<VarRule> |
                                                  ref<ProjectionRule>))) +
            many(append<&ast::SelectQuery::datasets>(ref<DatasetClauseRule>)) +
            option(ikw<"WHERE">) +
            assign<&ast::SelectQuery::where>(ref<GroupGraphPatternRule>) +
            assign<&ast::SelectQuery::modifier>(ref<SolutionModifierRule>))> {};

  static constexpr auto ConstructTriplesInto =
      some(append<&ast::ConstructQuery::templateTriples>(ref<TriplesRule>),
           lit<".">) +
      option(lit<".">);

  struct ConstructQueryRule
      : rule<create<ast::ConstructQuery>(
            ikw<"CONSTRUCT"> + lit<"{"> + option(ConstructTriplesInto) +
            lit<"}"> +
            many(append<&ast::ConstructQuery::datasets>(
                ref<DatasetClauseRule>)) +
            option(ikw<"WHERE">) +
            assign<&ast::ConstructQuery::where>(ref<GroupGraphPatternRule>) +
            assign<&ast::ConstructQuery::modifier>(
                ref<SolutionModifierRule>))> {};

  struct DescribeQueryRule
      : rule<create<ast::DescribeQuery>(
            ikw<"DESCRIBE"> +
            (flag<&ast::DescribeQuery::star>(lit<"*">) |
             some(append<&ast::DescribeQuery::vars>(VarOrIriRef))) +
            many(
                append<&ast::DescribeQuery::datasets>(ref<DatasetClauseRule>)) +
            option(option(ikw<"WHERE">) + assign<&ast::DescribeQuery::where>(
                                              ref<GroupGraphPatternRule>)) +
            assign<&ast::DescribeQuery::modifier>(ref<SolutionModifierRule>))> {
  };

  struct AskQueryRule
      : rule<create<ast::AskQuery>(
            ikw<"ASK"> +
            many(append<&ast::AskQuery::datasets>(ref<DatasetClauseRule>)) +
            option(ikw<"WHERE">) +
            assign<&ast::AskQuery::where>(ref<GroupGraphPatternRule>))> {};

  struct QueryRule : rule<create<ast::Query>(
                         assign<&ast::Query::prologue>(ref<PrologueRule>) +
                         assign<&ast::Query::form>(
                             ref<SelectQueryRule> | ref<ConstructQueryRule> |
                             ref<DescribeQueryRule> | ref<AskQueryRule>))> {};

  using entry = QueryRule;
  using ignore = list<WS, LINE_COMMENT>;
};

} // namespace sparql
