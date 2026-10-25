#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "xsdregex/core/ast.hpp"
namespace xsdregex {
struct XsdRegex : cursiv::GrammarSpec {
  struct Char : terminal<~cr<R"(.\\?*+()|[]{})">> {};
  struct WildcardEsc : terminal<lit<".">> {};
  struct SingleCharEsc : terminal<lit<"\\"> + cr<R"(nrt\\|.?*+(){}[]^-)">> {};
  struct MultiCharEsc : terminal<lit<"\\"> + cr<"sSiIcCdDwW">> {};
  struct UnknownEscape
      : terminal<lit<"\\"> + !cr<R"(-nrt\\|.?*+(){}[]^sSiIcCdDwW)"> +
                 option(any)> {};
  struct PropertyName : terminal<some(cr<"a-zA-Z0-9-">)> {};
  struct PropertyRule
      : rule<create<ast::CharProp>(
            (flag<&ast::CharProp::negated>(lit<"\\P{">) | lit<"\\p{">)+option(
                assign<&ast::CharProp::name>(ref<PropertyName>)) +
            lit<"}">)> {};
  struct XmlChar : terminal<~cr<R"([]\\-)">> {};
  struct SeRangeRule
      : rule<assign<&ast::CharRange::from>(ref<XmlChar> | ref<SingleCharEsc>) +
             lit<"-"> +
             assign<&ast::CharRange::to>(ref<XmlChar> | ref<SingleCharEsc>)> {};
  struct Dash : terminal<lit<"-"> + !lit<"[">> {};
  struct CharClassExprRule
      : rule<create<ast::CharClassExpr>(
            (flag<&ast::CharClassExpr::negated>(lit<"[^">) | lit<"[">)+many(
                append<&ast::CharClassExpr::items>(
                    ref<SeRangeRule> | ref<XmlChar> | ref<SingleCharEsc> |
                    ref<MultiCharEsc> | ref<PropertyRule> | ref<UnknownEscape> |
                    ref<Dash>)) +
            option(lit<"-"> + assign<&ast::CharClassExpr::subtract>(
                                  ref<CharClassExprRule>)) +
            lit<"]">)> {};
  struct QuantExact : terminal<some(cr<"0-9">)> {};
  struct Question : terminal<lit<"?">, value<ast::QuantifierKind::question>> {};
  struct Star : terminal<lit<"*">, value<ast::QuantifierKind::star>> {};
  struct Plus : terminal<lit<"+">, value<ast::QuantifierKind::plus>> {};
  struct QuantifierRule
      : rule<create<ast::Quantifier>(
            assign<&ast::Quantifier::kind>(ref<Question> | ref<Star> |
                                           ref<Plus>) |
            (lit<"{"> + option(assign<&ast::Quantifier::min>(ref<QuantExact>)) +
             option(flag<&ast::Quantifier::comma>(lit<",">) +
                    option(assign<&ast::Quantifier::max>(ref<QuantExact>))) +
             lit<"}">))> {};
  struct RegExpRule;
  struct ParenAtomRule : rule<lit<"("> + ref<RegExpRule> + lit<")">> {};
  struct PieceRule
      : rule<assign<&ast::Piece::atom>(
                 ref<Char> | ref<WildcardEsc> | ref<SingleCharEsc> |
                 ref<MultiCharEsc> | ref<PropertyRule> |
                 ref<CharClassExprRule> | ref<ParenAtomRule> |
                 ref<UnknownEscape>) +
             option(assign<&ast::Piece::quant>(ref<QuantifierRule>))> {};
  struct BranchRule : rule<create<ast::Branch>(
                          many(append<&ast::Branch::pieces>(ref<PieceRule>)))> {
  };
  struct RegExpRule
      : rule<create<ast::RegExp>(
            some(append<&ast::RegExp::branches>(ref<BranchRule>), lit<"|">))> {
  };
  using entry = RegExpRule;
};
} // namespace xsdregex
