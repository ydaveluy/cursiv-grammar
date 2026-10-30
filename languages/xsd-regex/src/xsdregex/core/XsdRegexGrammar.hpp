#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "xsdregex/core/ast.hpp"
namespace xsdregex {
struct XsdRegex : cursiv::GrammarSpec {
  static constexpr auto Scalar = cr<R"(\x00-\u{10FFFF})">;
  struct Char : terminal<~(cr<R"(.\\?*+()|[]{})"> | ~Scalar)> {};
  struct WildcardEsc : terminal<lit<".">> {};
  struct SingleCharEsc : terminal<lit<"\\"> + cr<R"(nrt\\|.?*+(){}[]^-)">> {};
  struct MultiCharEsc : terminal<lit<"\\"> + cr<"sSiIcCdDwW">> {};
  struct UnknownEscape
      : terminal<lit<"\\"> + !cr<R"(-nrt\\|.?*+(){}[]^sSiIcCdDwW)"> +
                 option(Scalar)> {};
  struct PropertyName : terminal<some(cr<"a-zA-Z0-9-">)> {};
  struct Property
      : rule<create<ast::CharProp>(
            (flag<&ast::CharProp::negated>(lit<"\\P{">) | lit<"\\p{">)+option(
                assign<&ast::CharProp::name>(ref<PropertyName>)) +
            lit<"}">)> {};
  struct XmlChar : terminal<~(cr<R"([]\\-)"> | ~Scalar)> {};
  struct SeRange
      : rule<assign<&ast::CharRange::from>(ref<XmlChar> | ref<SingleCharEsc>) +
             lit<"-"> +
             assign<&ast::CharRange::to>(ref<XmlChar> | ref<SingleCharEsc>)> {};
  struct Dash : terminal<lit<"-"> + !lit<"[">> {};
  struct CharClassExpr
      : rule<create<ast::CharClassExpr>(
            (flag<&ast::CharClassExpr::negated>(lit<"[^">) | lit<"[">)+many(
                append<&ast::CharClassExpr::items>(
                    ref<SeRange> | ref<XmlChar> | ref<SingleCharEsc> |
                    ref<MultiCharEsc> | ref<Property> | ref<UnknownEscape> |
                    ref<Dash>)) +
            option(lit<"-"> + assign<&ast::CharClassExpr::subtract>(
                                  ref<CharClassExpr>)) +
            lit<"]">)> {};
  struct QuantExact : terminal<some(cr<"0-9">)> {};
  struct Quantifier
      : rule<create<ast::Quantifier>(
            assign<&ast::Quantifier::kind>(as<ast::QuantifierKind::question>(lit<"?">) | as<ast::QuantifierKind::star>(lit<"*">) |
                                           as<ast::QuantifierKind::plus>(lit<"+">)) |
            (lit<"{"> + option(assign<&ast::Quantifier::min>(ref<QuantExact>)) +
             option(flag<&ast::Quantifier::comma>(lit<",">) +
                    option(assign<&ast::Quantifier::max>(ref<QuantExact>))) +
             lit<"}">))> {};
  struct RegExp;
  struct ParenAtom : rule<lit<"("> + ref<RegExp> + lit<")">> {};
  struct Piece
      : rule<assign<&ast::Piece::atom>(
                 ref<Char> | ref<WildcardEsc> | ref<SingleCharEsc> |
                 ref<MultiCharEsc> | ref<Property> |
                 ref<CharClassExpr> | ref<ParenAtom> |
                 ref<UnknownEscape>) +
             option(assign<&ast::Piece::quant>(ref<Quantifier>))> {};
  struct Branch : rule<create<ast::Branch>(
                          many(append<&ast::Branch::pieces>(ref<Piece>)))> {
  };
  struct RegExp
      : rule<create<ast::RegExp>(
            some(append<&ast::RegExp::branches>(ref<Branch>), lit<"|">))> {
  };
  using entry = RegExp;
};
} // namespace xsdregex
