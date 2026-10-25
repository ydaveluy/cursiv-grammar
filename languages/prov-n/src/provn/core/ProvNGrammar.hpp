// The provn language: its grammar. PROV-N (W3C provenance notation): a
// document of typed relation statements between entities, activities and
// agents, namespace prefixes, and nested bundles.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "provn/core/ast.hpp"

namespace provn {

inline constexpr auto strip1 = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Strip1 = cursiv::text_conv<strip1>;

inline constexpr auto strip_string = [](std::string_view t) -> std::string_view {
  if (t.size() >= 6 && t.substr(0, 3) == "\"\"\"") return t.substr(3, t.size() - 6);
  return t.substr(1, t.size() - 2);
};
using StripString = cursiv::text_conv<strip_string>;

inline constexpr auto strip_at = [](std::string_view t) { return t.substr(1); };
using StripAt = cursiv::text_conv<strip_at>;

struct ProvN : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n\f)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // Unicode ranges of PN_CHARS_BASE, spelled as code points -- same
  // production as Turtle's and SPARQL's NCName-derived PN_CHARS_BASE.
  static constexpr auto pn_chars_base =
      cr<"A-Za-zÀ-ÖØ-öø-˿Ͱ-ͽͿ-"
         "῿‌-‍⁰-↏Ⰰ-⿯、-퟿豈-"
         "﷏ﷰ-�">;
  static constexpr auto pn_chars_u = pn_chars_base | cr<"_">;
  static constexpr auto pn_chars =
      pn_chars_u | cr<"0-9·̀-ͯ‿-⁀-">;

  static constexpr auto pn_prefix =
      pn_chars_base + option(many(pn_chars | lit<".">) + pn_chars);

  static constexpr auto hex = cr<"0-9A-Fa-f">;
  static constexpr auto percent = lit<"%"> + repeat<2>(hex);
  static constexpr auto pn_chars_esc =
      lit<"\\"> + cr<"='(),:;[].-">;
  static constexpr auto pn_chars_others =
      cr<"/@~&+*?#$!"> | percent | pn_chars_esc;
  static constexpr auto pn_local =
      (pn_chars_u | cr<"0-9"> | pn_chars_others) +
      option(many(pn_chars | lit<"."> | pn_chars_others) +
             (pn_chars | pn_chars_others));

  // A prefix and a qualified name merge into one terminal:
  // a bare prefix's start and continuation sets both narrow a bare local
  // name's, so nothing either could match is lost by folding them together.
  static constexpr auto name_body =
      (pn_prefix + lit<":"> + option(pn_local)) | pn_local;
  struct Name : terminal<name_body> {};
  struct PrefixName : terminal<pn_prefix> {};

  static constexpr auto echar = lit<"\\"> + cr<R"(btnfr"'\\)">;
  static constexpr auto string_short =
      lit<"\""> + many(echar | cr<R"(^"\\\r\n)">) + lit<"\"">;
  static constexpr auto string_long =
      lit<"\"\"\""> +
      many(option(lit<"\"\""> | lit<"\"">) + (echar | cr<R"(^"\\)">)) +
      lit<"\"\"\"">;
  struct STRING_SHORT : terminal<string_short, Strip1> {};
  struct STRING_LONG : terminal<string_long, StripString> {};

  struct QNAME_LITERAL : terminal<lit<"'"> + name_body + lit<"'">, Strip1> {};

  struct INT_LITERAL
      : terminal<option(lit<"-">) + some(d), std::int64_t> {};

  struct LANGTAG
      : terminal<lit<"@"> + some(cr<"a-zA-Z">) +
                     many(lit<"-"> + some(cr<"a-zA-Z0-9">)),
                 StripAt> {};

  struct IRIREF
      : terminal<lit<"<"> +
                     many(cr<R"(^<>"{}|^`\\\x00- )">) + lit<">">,
                 Strip1> {};

  static constexpr auto year =
      option(lit<"-">) + ((cr<"1-9"> + d + d + some(d)) |
                          (lit<"0"> + d + d + d));
  static constexpr auto month = (lit<"0"> + cr<"1-9">) | (lit<"1"> + cr<"0-2">);
  static constexpr auto day =
      (lit<"0"> + cr<"1-9">) | (cr<"12"> + d) | (lit<"3"> + cr<"01">);
  static constexpr auto hour_min_sec =
      (((cr<"01"> + d) | (lit<"2"> + cr<"0-3">)) + lit<":"> + cr<"0-5"> + d +
       lit<":"> + cr<"0-5"> + d + option(lit<"."> + some(d))) |
      (lit<"24:00:00"> + option(lit<"."> + some(lit<"0">)));
  static constexpr auto timezone =
      option(lit<"Z"> | (cr<"+-"> + ((((cr<"01"> + d) | (lit<"1"> + cr<"0-3">)) +
                                     lit<":"> + cr<"0-5"> + d) |
                                     lit<"14:00">)));
  static constexpr auto datetime_body =
      year + lit<"-"> + month + lit<"-"> + day + lit<"T"> + hour_min_sec +
      timezone;
  struct DATETIME : terminal<datetime_body> {};

  struct AttrPairRule;
  struct LiteralRule;
  struct EntityExprRule;
  struct ActivityExprRule;
  struct GenerationExprRule;
  struct UsageExprRule;
  struct StartExprRule;
  struct EndExprRule;
  struct InvalidationExprRule;
  struct CommunicationExprRule;
  struct AgentExprRule;
  struct AssociationExprRule;
  struct AttributionExprRule;
  struct DelegationExprRule;
  struct DerivationExprRule;
  struct InfluenceExprRule;
  struct AlternateExprRule;
  struct SpecializationExprRule;
  struct MembershipExprRule;
  struct ExtensibilityExprRule;
  struct ExtArgRule;
  struct BundleRule;

  static constexpr auto ExpressionAlt =
      ref<EntityExprRule> | ref<ActivityExprRule> | ref<GenerationExprRule> |
      ref<UsageExprRule> | ref<StartExprRule> | ref<EndExprRule> |
      ref<InvalidationExprRule> | ref<CommunicationExprRule> |
      ref<AgentExprRule> | ref<AssociationExprRule> |
      ref<AttributionExprRule> | ref<DelegationExprRule> |
      ref<DerivationExprRule> | ref<InfluenceExprRule> |
      ref<AlternateExprRule> | ref<SpecializationExprRule> |
      ref<MembershipExprRule> | ref<ExtensibilityExprRule>;

  struct AttrPairRule
      : rule<assign<&ast::AttributeValuePair::attribute>(ref<Name>) +
             lit<"="> +
             assign<&ast::AttributeValuePair::value>(ref<LiteralRule>)> {};

  struct LiteralRule
      : rule<(assign<&ast::TypedLiteral::text>(ref<STRING_LONG> |
                                               ref<STRING_SHORT>) +
              lit<"%%"> +
              assign<&ast::TypedLiteral::datatype>(ref<Name>)) |
             (assign<&ast::StringLiteral::text>(ref<STRING_LONG> |
                                                ref<STRING_SHORT>) +
              option(assign<&ast::StringLiteral::lang>(ref<LANGTAG>))) |
             assign<&ast::IntLiteral::value>(ref<INT_LITERAL>) |
             assign<&ast::QualifiedNameLiteral::value>(
                 ref<QNAME_LITERAL>)> {};

  struct EntityExprRule
      : rule<kw<"entity"> + lit<"("> +
             assign<&ast::EntityExpr::entity>(ref<Name>) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::EntityExpr::attributes>(
                             ref<AttrPairRule>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct ActivityExprRule
      : rule<kw<"activity"> + lit<"("> +
             assign<&ast::ActivityExpr::activity>(ref<Name>) +
             option(lit<","> +
                    (assign<&ast::ActivityExpr::startTime>(ref<DATETIME>) |
                     lit<"-">) +
                    lit<","> +
                    (assign<&ast::ActivityExpr::endTime>(ref<DATETIME>) |
                     lit<"-">)) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::ActivityExpr::attributes>(
                             ref<AttrPairRule>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct GenerationExprRule
      : rule<kw<"wasGeneratedBy"> + lit<"("> +
             option((assign<&ast::GenerationExpr::id>(ref<Name>) |
                     lit<"-">) +
                    lit<";">) +
             assign<&ast::GenerationExpr::entity>(ref<Name>) +
             option(lit<","> +
                    (assign<&ast::GenerationExpr::activity>(ref<Name>) |
                     lit<"-">) +
                    lit<","> +
                    (assign<&ast::GenerationExpr::time>(ref<DATETIME>) |
                     lit<"-">)) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::GenerationExpr::attributes>(
                             ref<AttrPairRule>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct UsageExprRule
      : rule<kw<"used"> + lit<"("> +
             option((assign<&ast::UsageExpr::id>(ref<Name>) | lit<"-">) +
                    lit<";">) +
             assign<&ast::UsageExpr::activity>(ref<Name>) +
             option(lit<","> +
                    (assign<&ast::UsageExpr::entity>(ref<Name>) |
                     lit<"-">) +
                    lit<","> +
                    (assign<&ast::UsageExpr::time>(ref<DATETIME>) |
                     lit<"-">)) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::UsageExpr::attributes>(
                             ref<AttrPairRule>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct StartExprRule
      : rule<kw<"wasStartedBy"> + lit<"("> +
             option((assign<&ast::StartExpr::id>(ref<Name>) | lit<"-">) +
                    lit<";">) +
             assign<&ast::StartExpr::activity>(ref<Name>) +
             option(lit<","> +
                    (assign<&ast::StartExpr::trigger>(ref<Name>) |
                     lit<"-">) +
                    lit<","> +
                    (assign<&ast::StartExpr::starter>(ref<Name>) |
                     lit<"-">) +
                    lit<","> +
                    (assign<&ast::StartExpr::time>(ref<DATETIME>) |
                     lit<"-">)) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::StartExpr::attributes>(
                             ref<AttrPairRule>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct EndExprRule
      : rule<kw<"wasEndedBy"> + lit<"("> +
             option((assign<&ast::EndExpr::id>(ref<Name>) | lit<"-">) +
                    lit<";">) +
             assign<&ast::EndExpr::activity>(ref<Name>) +
             option(lit<","> +
                    (assign<&ast::EndExpr::trigger>(ref<Name>) | lit<"-">) +
                    lit<","> +
                    (assign<&ast::EndExpr::ender>(ref<Name>) | lit<"-">) +
                    lit<","> +
                    (assign<&ast::EndExpr::time>(ref<DATETIME>) |
                     lit<"-">)) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::EndExpr::attributes>(ref<AttrPairRule>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct InvalidationExprRule
      : rule<kw<"wasInvalidatedBy"> + lit<"("> +
             option((assign<&ast::InvalidationExpr::id>(ref<Name>) |
                     lit<"-">) +
                    lit<";">) +
             assign<&ast::InvalidationExpr::entity>(ref<Name>) +
             option(lit<","> +
                    (assign<&ast::InvalidationExpr::activity>(ref<Name>) |
                     lit<"-">) +
                    lit<","> +
                    (assign<&ast::InvalidationExpr::time>(ref<DATETIME>) |
                     lit<"-">)) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::InvalidationExpr::attributes>(
                             ref<AttrPairRule>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct CommunicationExprRule
      : rule<kw<"wasInformedBy"> + lit<"("> +
             option((assign<&ast::CommunicationExpr::id>(ref<Name>) |
                     lit<"-">) +
                    lit<";">) +
             assign<&ast::CommunicationExpr::informed>(ref<Name>) +
             lit<","> +
             assign<&ast::CommunicationExpr::informant>(ref<Name>) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::CommunicationExpr::attributes>(
                             ref<AttrPairRule>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct AgentExprRule
      : rule<kw<"agent"> + lit<"("> +
             assign<&ast::AgentExpr::agent>(ref<Name>) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::AgentExpr::attributes>(
                             ref<AttrPairRule>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct AssociationExprRule
      : rule<kw<"wasAssociatedWith"> + lit<"("> +
             option((assign<&ast::AssociationExpr::id>(ref<Name>) |
                     lit<"-">) +
                    lit<";">) +
             assign<&ast::AssociationExpr::activity>(ref<Name>) +
             option(lit<","> +
                    (assign<&ast::AssociationExpr::agent>(ref<Name>) |
                     lit<"-">) +
                    lit<","> +
                    (assign<&ast::AssociationExpr::plan>(ref<Name>) |
                     lit<"-">)) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::AssociationExpr::attributes>(
                             ref<AttrPairRule>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct AttributionExprRule
      : rule<kw<"wasAttributedTo"> + lit<"("> +
             option((assign<&ast::AttributionExpr::id>(ref<Name>) |
                     lit<"-">) +
                    lit<";">) +
             assign<&ast::AttributionExpr::entity>(ref<Name>) + lit<","> +
             assign<&ast::AttributionExpr::agent>(ref<Name>) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::AttributionExpr::attributes>(
                             ref<AttrPairRule>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct DelegationExprRule
      : rule<kw<"actedOnBehalfOf"> + lit<"("> +
             option((assign<&ast::DelegationExpr::id>(ref<Name>) |
                     lit<"-">) +
                    lit<";">) +
             assign<&ast::DelegationExpr::delegate>(ref<Name>) + lit<","> +
             assign<&ast::DelegationExpr::responsible>(ref<Name>) +
             option(lit<","> +
                    (assign<&ast::DelegationExpr::activity>(ref<Name>) |
                     lit<"-">)) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::DelegationExpr::attributes>(
                             ref<AttrPairRule>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct DerivationExprRule
      : rule<kw<"wasDerivedFrom"> + lit<"("> +
             option((assign<&ast::DerivationExpr::id>(ref<Name>) |
                     lit<"-">) +
                    lit<";">) +
             assign<&ast::DerivationExpr::generatedEntity>(ref<Name>) +
             lit<","> +
             assign<&ast::DerivationExpr::usedEntity>(ref<Name>) +
             option(lit<","> +
                    (assign<&ast::DerivationExpr::activity>(ref<Name>) |
                     lit<"-">) +
                    lit<","> +
                    (assign<&ast::DerivationExpr::generation>(ref<Name>) |
                     lit<"-">) +
                    lit<","> +
                    (assign<&ast::DerivationExpr::usage>(ref<Name>) |
                     lit<"-">)) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::DerivationExpr::attributes>(
                             ref<AttrPairRule>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct InfluenceExprRule
      : rule<kw<"wasInfluencedBy"> + lit<"("> +
             option((assign<&ast::InfluenceExpr::id>(ref<Name>) |
                     lit<"-">) +
                    lit<";">) +
             assign<&ast::InfluenceExpr::influencee>(ref<Name>) + lit<","> +
             assign<&ast::InfluenceExpr::influencer>(ref<Name>) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::InfluenceExpr::attributes>(
                             ref<AttrPairRule>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct AlternateExprRule
      : rule<kw<"alternateOf"> + lit<"("> +
             assign<&ast::AlternateExpr::entity1>(ref<Name>) + lit<","> +
             assign<&ast::AlternateExpr::entity2>(ref<Name>) + lit<")">> {};

  struct SpecializationExprRule
      : rule<kw<"specializationOf"> + lit<"("> +
             assign<&ast::SpecializationExpr::specificEntity>(ref<Name>) +
             lit<","> +
             assign<&ast::SpecializationExpr::generalEntity>(ref<Name>) +
             lit<")">> {};

  struct MembershipExprRule
      : rule<kw<"hadMember"> + lit<"("> +
             assign<&ast::MembershipExpr::collection>(ref<Name>) + lit<","> +
             assign<&ast::MembershipExpr::member>(ref<Name>) + lit<")">> {};

  struct ExtArgRule
      : rule<assign<&ast::ExtArgLiteral::value>(ref<LiteralRule>) |
             assign<&ast::ExtArgTime::value>(ref<DATETIME>) |
             assign<&ast::ExtArgExpression::value>(
                 ref<ExtensibilityExprRule>) |
             (create<ast::ExtArgTuple> +
              ((flag<&ast::ExtArgTuple::braces>(lit<"{">) +
                some(append<&ast::ExtArgTuple::items>(ref<ExtArgRule>),
                     lit<",">) +
                lit<"}">) |
               (lit<"("> +
                some(append<&ast::ExtArgTuple::items>(ref<ExtArgRule>),
                     lit<",">) +
                lit<")">))) |
             (create<ast::ExtArgIdentifier> +
              (assign<&ast::ExtArgIdentifier::value>(ref<Name>) |
               lit<"-">))> {};

  struct ExtensibilityExprRule
      : rule<create<ast::ExtensibilityExpr> +
             assign<&ast::ExtensibilityExpr::name>(ref<Name>) + lit<"("> +
             option((assign<&ast::ExtensibilityExpr::id>(ref<Name>) |
                     lit<"-">) +
                    lit<";">) +
             some(append<&ast::ExtensibilityExpr::args>(ref<ExtArgRule>),
                  lit<",">) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::ExtensibilityExpr::attributes>(
                             ref<AttrPairRule>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct PrefixDeclRule
      : rule<kw<"prefix"> +
             assign<&ast::PrefixDecl::prefix>(ref<PrefixName>) +
             assign<&ast::PrefixDecl::iri>(ref<IRIREF>)> {};

  struct BundleRule
      : rule<kw<"bundle"> + assign<&ast::Bundle::identifier>(ref<Name>) +
             option(((kw<"default"> +
                     assign<&ast::Bundle::defaultNamespace>(ref<IRIREF>)) |
                    append<&ast::Bundle::prefixes>(ref<PrefixDeclRule>)) +
                    many(append<&ast::Bundle::prefixes>(ref<PrefixDeclRule>))) +
             many(append<&ast::Bundle::expressions>(ExpressionAlt)) +
             kw<"endBundle">> {};

  struct DocumentRule
      : rule<create<ast::Document> + kw<"document"> +
             option(((kw<"default"> +
                     assign<&ast::Document::defaultNamespace>(ref<IRIREF>)) |
                    append<&ast::Document::prefixes>(ref<PrefixDeclRule>)) +
                    many(append<&ast::Document::prefixes>(
                        ref<PrefixDeclRule>))) +
             many(append<&ast::Document::expressions>(ExpressionAlt)) +
             many(append<&ast::Document::bundles>(ref<BundleRule>)) +
             kw<"endDocument">> {};

  using entry = DocumentRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace provn
