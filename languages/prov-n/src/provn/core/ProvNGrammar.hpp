// The provn language: its grammar. PROV-N (W3C provenance notation): a
// document of typed relation statements between entities, activities and
// agents, namespace prefixes, and nested bundles.
#pragma once

#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "provn/core/ast.hpp"

namespace provn {

inline constexpr auto strip1 = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

inline constexpr auto unescape = [](std::string_view t,
    std::pmr::memory_resource *mem) -> std::string_view {
  const std::size_t quotes = t.starts_with("\"\"\"") ? 3 : 1;
  const auto body = t.substr(quotes, t.size() - 2 * quotes);
  if (body.find('\\') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] != '\\') { room[n++] = body[i]; continue; }
    const char e = body[++i];
    switch (e) {
      case 'b': room[n++] = '\b'; break;
      case 't': room[n++] = '\t'; break;
      case 'n': room[n++] = '\n'; break;
      case 'f': room[n++] = '\f'; break;
      case 'r': room[n++] = '\r'; break;
      case '"': case '\'': case '\\': room[n++] = e; break;
      default: {
        unsigned c = 0;
        for (unsigned j = 0; j < (e == 'u' ? 4u : 8u); ++j) {
          const char h = body[++i];
          c = c * 16 + (h <= '9' ? h - '0' : (h | 32) - 'a' + 10);
        }
        if (c < 0x80) room[n++] = static_cast<char>(c);
        else {
          const unsigned bytes = c < 0x800 ? 2 : c < 0x10000 ? 3 : 4;
          room[n++] = static_cast<char>((0xffu << (8 - bytes)) | (c >> (6 * (bytes - 1))));
          for (unsigned j = bytes - 1; j > 0; --j)
            room[n++] = static_cast<char>(0x80 | ((c >> (6 * (j - 1))) & 63));
        }
      }
    }
  }
  return std::string_view(room.data(), n);
};

inline constexpr auto strip_at = [](std::string_view t) { return t.substr(1); };

inline constexpr auto datetime_value = [](std::string_view t) {
  using Result = cursiv::ConversionResult<std::string_view>;
  const auto first = t.starts_with("-") ? 1u : 0u;
  const auto end = t.find('-', first);
  unsigned year = 0;
  for (std::size_t i = first; i < end; ++i) year = (year * 10 + t[i] - '0') % 400;
  const unsigned month = (t[end + 1] - '0') * 10 + t[end + 2] - '0';
  const unsigned day = (t[end + 4] - '0') * 10 + t[end + 5] - '0';
  constexpr unsigned days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  const bool leap = year % 4 == 0 && (year % 100 != 0 || year == 0);
  if (day > days[month - 1] + (month == 2 && leap ? 1u : 0u))
    return Result::err("Invalid Gregorian day in PROV-N dateTime.");
  return Result::ok(t);
};

struct ProvN : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n\f)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // Unicode ranges of PN_CHARS_BASE, spelled as code points -- same
  // production as Turtle's and SPARQL's NCName-derived PN_CHARS_BASE.
  static constexpr auto pn_chars_base =
      cr<"A-Za-z\u00c0-\u00d6\u00d8-\u00f6\u00f8-\u02ff"
         "\u0370-\u037d\u037f-\u1fff\u200c-\u200d\u2070-\u218f"
         "\u2c00-\u2fef\u3001-\ud7ff\uf900-\ufdcf\ufdf0-\ufffd"
         "\U00010000-\U000effff">;
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

  static constexpr auto scalar4 = (cr<"0-9a-cA-Ce-fE-F"> + repeat<3>(hex)) |
      (cr<"dD"> + cr<"0-7"> + repeat<2>(hex));
  static constexpr auto scalar8 = (lit<"0000"> + scalar4) |
      (lit<"000"> + cr<"1-9a-fA-F"> + repeat<4>(hex)) |
      (lit<"0010"> + repeat<4>(hex));
  static constexpr auto uchar = lit<"\\"> +
      ((lit<"u"> + scalar4) | (lit<"U"> + scalar8));
  static constexpr auto echar = (lit<"\\"> + cr<R"(btnfr"'\\)">) | uchar;
  static constexpr auto string_short =
      lit<"\""> + many(echar | cr<R"(^"\\\r\n)">) + lit<"\"">;
  static constexpr auto string_long =
      lit<"\"\"\""> +
      many(option(lit<"\"\""> | lit<"\"">) + (echar | cr<R"(^"\\)">)) +
      lit<"\"\"\"">;
  struct STRING_SHORT : terminal<as<unescape>(string_short)> {};
  struct STRING_LONG : terminal<as<unescape>(string_long)> {};

  struct QNAME_LITERAL : terminal<as<strip1>(lit<"'"> + name_body + lit<"'">)> {
  };

  struct INT_LITERAL : terminal<option(lit<"-">) + some(d)> {
  };

  struct LANGTAG
      : terminal<as<strip_at>(lit<"@"> + some(cr<"a-zA-Z">) +
                              many(lit<"-"> + some(cr<"a-zA-Z0-9">)))> {};

  struct IRIREF : terminal<as<unescape>(
                      lit<"<"> + many(uchar | cr<R"(^<>"{}|^`\\\x00- )">) + lit<">">)> {
  };

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
      option(lit<"Z"> | (cr<"+-"> + ((((lit<"0"> + d) | (lit<"1"> + cr<"0-3">)) +
                                     lit<":"> + cr<"0-5"> + d) |
                                     lit<"14:00">)));
  static constexpr auto datetime_body =
      year + lit<"-"> + month + lit<"-"> + day + lit<"T"> + hour_min_sec +
      timezone;
  struct DATETIME : terminal<as<datetime_value>(datetime_body)> {};

  struct AttrPair;
  struct Literal;
  struct EntityExpr;
  struct ActivityExpr;
  struct GenerationExpr;
  struct UsageExpr;
  struct StartExpr;
  struct EndExpr;
  struct InvalidationExpr;
  struct CommunicationExpr;
  struct AgentExpr;
  struct AssociationExpr;
  struct AttributionExpr;
  struct DelegationExpr;
  struct DerivationExpr;
  struct InfluenceExpr;
  struct AlternateExpr;
  struct SpecializationExpr;
  struct MembershipExpr;
  struct ExtensibilityExpr;
  struct ExtArg;
  struct Bundle;

  static constexpr auto ExpressionAlt =
      ref<EntityExpr> | ref<ActivityExpr> | ref<GenerationExpr> |
      ref<UsageExpr> | ref<StartExpr> | ref<EndExpr> |
      ref<InvalidationExpr> | ref<CommunicationExpr> |
      ref<AgentExpr> | ref<AssociationExpr> |
      ref<AttributionExpr> | ref<DelegationExpr> |
      ref<DerivationExpr> | ref<InfluenceExpr> |
      ref<AlternateExpr> | ref<SpecializationExpr> |
      ref<MembershipExpr> | ref<ExtensibilityExpr>;

  struct AttrPair
      : rule<assign<&ast::AttributeValuePair::attribute>(ref<Name>) +
             lit<"="> +
             assign<&ast::AttributeValuePair::value>(ref<Literal>)> {};

  struct Literal
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

  struct EntityExpr
      : rule<kw<"entity"> + lit<"("> +
             assign<&ast::EntityExpr::entity>(ref<Name>) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::EntityExpr::attributes>(
                             ref<AttrPair>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct ActivityExpr
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
                             ref<AttrPair>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct GenerationExpr
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
                             ref<AttrPair>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct UsageExpr
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
                             ref<AttrPair>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct StartExpr
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
                             ref<AttrPair>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct EndExpr
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
                    many(append<&ast::EndExpr::attributes>(ref<AttrPair>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct InvalidationExpr
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
                             ref<AttrPair>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct CommunicationExpr
      : rule<kw<"wasInformedBy"> + lit<"("> +
             option((assign<&ast::CommunicationExpr::id>(ref<Name>) |
                     lit<"-">) +
                    lit<";">) +
             assign<&ast::CommunicationExpr::informed>(ref<Name>) +
             lit<","> +
             assign<&ast::CommunicationExpr::informant>(ref<Name>) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::CommunicationExpr::attributes>(
                             ref<AttrPair>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct AgentExpr
      : rule<kw<"agent"> + lit<"("> +
             assign<&ast::AgentExpr::agent>(ref<Name>) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::AgentExpr::attributes>(
                             ref<AttrPair>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct AssociationExpr
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
                             ref<AttrPair>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct AttributionExpr
      : rule<kw<"wasAttributedTo"> + lit<"("> +
             option((assign<&ast::AttributionExpr::id>(ref<Name>) |
                     lit<"-">) +
                    lit<";">) +
             assign<&ast::AttributionExpr::entity>(ref<Name>) + lit<","> +
             assign<&ast::AttributionExpr::agent>(ref<Name>) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::AttributionExpr::attributes>(
                             ref<AttrPair>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct DelegationExpr
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
                             ref<AttrPair>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct DerivationExpr
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
                             ref<AttrPair>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct InfluenceExpr
      : rule<kw<"wasInfluencedBy"> + lit<"("> +
             option((assign<&ast::InfluenceExpr::id>(ref<Name>) |
                     lit<"-">) +
                    lit<";">) +
             assign<&ast::InfluenceExpr::influencee>(ref<Name>) + lit<","> +
             assign<&ast::InfluenceExpr::influencer>(ref<Name>) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::InfluenceExpr::attributes>(
                             ref<AttrPair>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct AlternateExpr
      : rule<kw<"alternateOf"> + lit<"("> +
             assign<&ast::AlternateExpr::entity1>(ref<Name>) + lit<","> +
             assign<&ast::AlternateExpr::entity2>(ref<Name>) + lit<")">> {};

  struct SpecializationExpr
      : rule<kw<"specializationOf"> + lit<"("> +
             assign<&ast::SpecializationExpr::specificEntity>(ref<Name>) +
             lit<","> +
             assign<&ast::SpecializationExpr::generalEntity>(ref<Name>) +
             lit<")">> {};

  struct MembershipExpr
      : rule<kw<"hadMember"> + lit<"("> +
             assign<&ast::MembershipExpr::collection>(ref<Name>) + lit<","> +
             assign<&ast::MembershipExpr::member>(ref<Name>) + lit<")">> {};

  struct ExtArg
      : rule<assign<&ast::ExtArgLiteral::value>(ref<Literal>) |
             assign<&ast::ExtArgTime::value>(ref<DATETIME>) |
             assign<&ast::ExtArgExpression::value>(
                 ref<ExtensibilityExpr>) |
             (create<ast::ExtArgTuple> +
              ((flag<&ast::ExtArgTuple::braces>(lit<"{">) +
                some(append<&ast::ExtArgTuple::items>(ref<ExtArg>),
                     lit<",">) +
                lit<"}">) |
               (lit<"("> +
                some(append<&ast::ExtArgTuple::items>(ref<ExtArg>),
                     lit<",">) +
                lit<")">))) |
             (create<ast::ExtArgIdentifier> +
              (assign<&ast::ExtArgIdentifier::value>(ref<Name>) |
               lit<"-">))> {};

  struct ExtensibilityExpr
      : rule<create<ast::ExtensibilityExpr> +
             assign<&ast::ExtensibilityExpr::name>(ref<Name>) + lit<"("> +
             option((assign<&ast::ExtensibilityExpr::id>(ref<Name>) |
                     lit<"-">) +
                    lit<";">) +
             some(append<&ast::ExtensibilityExpr::args>(ref<ExtArg>),
                  lit<",">) +
             option(lit<","> + lit<"["> +
                    many(append<&ast::ExtensibilityExpr::attributes>(
                             ref<AttrPair>),
                         lit<",">) +
                    lit<"]">) +
             lit<")">> {};

  struct PrefixDecl
      : rule<kw<"prefix"> +
             assign<&ast::PrefixDecl::prefix>(ref<PrefixName>) +
             assign<&ast::PrefixDecl::iri>(ref<IRIREF>)> {};

  struct Bundle
      : rule<kw<"bundle"> + assign<&ast::Bundle::identifier>(ref<Name>) +
             option(((kw<"default"> +
                     assign<&ast::Bundle::defaultNamespace>(ref<IRIREF>)) |
                    append<&ast::Bundle::prefixes>(ref<PrefixDecl>)) +
                    many(append<&ast::Bundle::prefixes>(ref<PrefixDecl>))) +
             many(append<&ast::Bundle::expressions>(ExpressionAlt)) +
             kw<"endBundle">> {};

  struct Document
      : rule<create<ast::Document> + kw<"document"> +
             option(((kw<"default"> +
                     assign<&ast::Document::defaultNamespace>(ref<IRIREF>)) |
                    append<&ast::Document::prefixes>(ref<PrefixDecl>)) +
                    many(append<&ast::Document::prefixes>(
                        ref<PrefixDecl>))) +
             many(append<&ast::Document::expressions>(ExpressionAlt)) +
             many(append<&ast::Document::bundles>(ref<Bundle>)) +
             kw<"endDocument">> {};

  using entry = Document;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace provn
