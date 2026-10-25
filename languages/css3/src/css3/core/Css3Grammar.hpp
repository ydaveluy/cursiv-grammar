// The css3 language: its grammar. Selectors, at-rules and declaration
// values share one ComponentValue alternation (ast.hpp); paired
// known/unknown and good/bad rule alternatives are unified into one richer
// shape each -- both to avoid LR/GLR ambiguity between two rules accepting
// overlapping token runs, and because the pairing is a fallback artifact of
// top-down predictive parsing, not a real distinction in the language.
//
// Three constructs -- the descendant combinator, `calc()`'s `+`/`-`, and
// `@supports`'s not/and/or -- need a real gap between tokens where a plain
// `ws` step would already have consumed it;
// each is restructured to read the raw gap instead.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "css3/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace css3 {

inline constexpr auto unquote = [](std::string_view t,
                                  std::pmr::memory_resource* mem) {
  const auto body = t.substr(1, t.size() - 2);
  if (body.find_first_of("\\\r\f\0", 0, 4) == std::string_view::npos)
    return body;
  auto out = cursiv::text_room(mem, body.size() * 3);
  std::size_t n = 0;
  const auto hex = [](char c) -> unsigned {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 16;
  };
  const auto scalar = [&](std::uint32_t c) {
    if (c == 0 || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff)) c = 0xfffd;
    if (c < 0x80) out[n++] = static_cast<char>(c);
    else {
      const unsigned bytes = c < 0x800 ? 2 : c < 0x10000 ? 3 : 4;
      out[n++] = static_cast<char>((0xffu << (8 - bytes)) | (c >> (6 * (bytes - 1))));
      for (unsigned j = bytes - 1; j > 0; --j)
        out[n++] = static_cast<char>(0x80 | ((c >> (6 * (j - 1))) & 63));
    }
  };
  for (std::size_t i = 0; i < body.size(); ++i) {
    char c = body[i];
    if (c == '\\' && i + 1 < body.size()) {
      c = body[++i];
      if (c == '\r' || c == '\n' || c == '\f') {
        if (c == '\r' && i + 1 < body.size() && body[i + 1] == '\n') ++i;
        continue;
      }
      if (hex(c) < 16) {
        std::uint32_t value = hex(c);
        for (unsigned j = 1; j < 6 && i + 1 < body.size() && hex(body[i + 1]) < 16; ++j)
          value = value * 16 + hex(body[++i]);
        if (i + 1 < body.size() && body.substr(i + 1, 1).find_first_of(" \t\r\n\f") != std::string_view::npos) {
          c = body[++i];
          if (c == '\r' && i + 1 < body.size() && body[i + 1] == '\n') ++i;
        }
        scalar(value);
        continue;
      }
    }
    if (c == 0) scalar(0);
    else if (c == '\r' || c == '\f') {
      if (c == '\r' && i + 1 < body.size() && body[i + 1] == '\n') ++i;
      out[n++] = '\n';
    } else out[n++] = c;
  }
  return std::string_view(out.data(), n);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Css3 : cursiv::GrammarSpec {
  // `-` is a legal identifier byte throughout CSS (`font-size`), so the
  // keyword boundary must include it or `kw<"only">` would run into `-only`.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_-">;

  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct SPACE : terminal<some(cr<R"( \t\r\n\f)">)> {};
  // Legacy HTML-comment-hiding markers: no rule needs their position, only
  // that they are ignored wherever they appear.
  struct CDO : terminal<lit<"<!--">> {};
  struct CDC : terminal<lit<"-->">> {};

  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  static constexpr auto IdentEscape = lit<"\\"> +
      ((repeat<1, 6>(HexDigit) + option(lit<"\r\n"> | cr<R"( \t\r\n\f)">)) |
       cr<R"(^0-9a-fA-F\r\n\f)">);
  static constexpr auto IdentStart =
      ~cr<R"(\x00-@[-^`{-\x7f)"> | IdentEscape;
  static constexpr auto IdentCont =
      ~cr<R"(\x00-,.-/:-@[-^`{-\x7f)"> |
      IdentEscape;
  struct IDENT
      : terminal<(IdentStart | (lit<"-"> + (IdentStart | lit<"-">))) +
                 many(IdentCont)> {};
  struct AT_KEYWORD
      : terminal<lit<"@"> + ref<IDENT>> {
  };

  static constexpr auto NumberBody =
      ((some(d) + option(lit<"."> + some(d))) | (lit<"."> + some(d))) +
      option(cr<"eE"> + option(cr<"+-">) + some(d));
  struct NUMBER : terminal<NumberBody, double> {};
  struct PERCENTAGE : terminal<NumberBody + lit<"%">> {};

  static constexpr auto UnitSpelling =
      ikw<"vmin"> | ikw<"vmax"> | ikw<"vw"> | ikw<"vh"> | ikw<"rem"> |
      ikw<"em"> | ikw<"ex"> | ikw<"ch"> | ikw<"px"> | ikw<"cm"> | ikw<"mm"> |
      ikw<"in"> | ikw<"pt"> | ikw<"pc"> | ikw<"q"> | ikw<"grad"> |
      ikw<"turn"> | ikw<"rad"> | ikw<"deg"> | ikw<"khz"> | ikw<"hz"> |
      ikw<"ms"> | ikw<"s"> | ikw<"dpcm"> | ikw<"dppx"> | ikw<"dpi">;
  struct DIMENSION : terminal<NumberBody + UnitSpelling> {};
  struct UNKNOWN_DIMENSION
      : terminal<NumberBody + IdentStart + many(IdentCont)> {};

  struct HASH : terminal<lit<"#"> + some(IdentCont)> {};

  static constexpr auto StringBody1 =
      many((lit<"\\"> + (lit<"\r\n"> | any)) | cr<R"(^"\\\r\n\f)">);
  static constexpr auto StringBody2 =
      many((lit<"\\"> + (lit<"\r\n"> | any)) | cr<R"(^'\\\r\n\f)">);
  struct STRING
      : terminal<(lit<"\""> + StringBody1 + lit<"\"">) |
                     (lit<"'"> + StringBody2 + lit<"'">),
                 Unquoted> {};

  // Real CSS3 unicode-range syntax (W3C CSS Values): 1-6 hex digits or `?`
  // wildcards, then an optional `-` and up to 6 more hex digits.
  struct UNICODE_RANGE
      : terminal<cr<"uU"> + lit<"+"> + repeat<1, 6>(HexDigit | lit<"?">) +
                 option(lit<"-"> + repeat<1, 6>(HexDigit))> {};

  static constexpr auto UrlRawByte =
      ~cr<R"(\x00- "'-)\x7f)"> | IdentEscape;
  struct URL_RAW : terminal<some(UrlRawByte)> {};

  struct IMPORTANT
      : terminal<lit<"!"> +
                 many(cr<R"( \t\r\n\f)"> | through(lit<"/*">, lit<"*/">)) +
                 ikw<"important">> {};

  static constexpr auto SpaceByte = cr<R"( \t\r\n\f)">;

  struct STAR : terminal<lit<"*">> {};
  struct PIPE : terminal<lit<"|">> {};
  struct MINUS : terminal<lit<"-">> {};
  struct PLUS : terminal<lit<"+">> {};

  struct COMBINE_CHILD : terminal<lit<">">, value<ast::Combinator::child>> {};
  struct COMBINE_ADJ
      : terminal<lit<"+">, value<ast::Combinator::adjacent_sibling>> {};
  struct COMBINE_SIB
      : terminal<lit<"~">, value<ast::Combinator::general_sibling>> {};

  struct PREFIX_MATCH : terminal<lit<"^=">, value<ast::AttribOp::prefix>> {};
  struct SUFFIX_MATCH : terminal<lit<"$=">, value<ast::AttribOp::suffix>> {};
  struct SUBSTRING_MATCH
      : terminal<lit<"*=">, value<ast::AttribOp::substring>> {};
  struct INCLUDES : terminal<lit<"~=">, value<ast::AttribOp::includes>> {};
  struct DASHMATCH : terminal<lit<"|=">, value<ast::AttribOp::dashmatch>> {};
  struct ATTR_EQ : terminal<lit<"=">, value<ast::AttribOp::equals>> {};
  static constexpr auto AttribOpAlt = ref<PREFIX_MATCH> | ref<SUFFIX_MATCH> |
                                       ref<SUBSTRING_MATCH> | ref<INCLUDES> |
                                       ref<DASHMATCH> | ref<ATTR_EQ>;

  struct CALC_ADD : terminal<lit<"+">, value<ast::CalcOp::add>> {};
  struct CALC_SUB : terminal<lit<"-">, value<ast::CalcOp::subtract>> {};
  struct CALC_MUL : terminal<lit<"*">, value<ast::CalcOp::multiply>> {};
  struct CALC_DIV : terminal<lit<"/">, value<ast::CalcOp::divide>> {};

  struct COLON_TOK : terminal<lit<":">> {};
  // Plain-text twins of INCLUDES/DASHMATCH: those carry an AttribOp
  // converter (needed for attribute selectors), which a string_view member
  // cannot also receive -- a bare `~=`/`|=` outside an attribute selector
  // needs the raw spelling.
  struct INCLUDES_TXT : terminal<lit<"~=">> {};
  struct DASHMATCH_TXT : terminal<lit<"|=">> {};

  // A name the language also uses as a fixed keyword in a few clauses
  // (media/supports/keyframe): spelled once, per the soft-keyword idiom --
  // IDENT already covers these bytes, the clauses below additionally read
  // them as keywords in their own states. A rule of its own (not a
  // `static constexpr auto` spliced at each of its many call sites): the
  // alternation would otherwise be recompiled once per site.
  // `var`/`calc`/`url` are ALSO fixed introducers elsewhere (VarRefRule,
  // CalcExprRule, UrlRefRule, each `ikw<"...."> + lit<"(">`) -- spelled
  // once here too, or the state offering both readings would commit to the
  // keyword and never fall back to a plain name when no `(` follows.
  struct IdentLikeRule
      : rule<ref<IDENT> | ikw<"only"> | ikw<"not"> | ikw<"and"> | ikw<"or"> |
             ikw<"from"> | ikw<"to"> | ikw<"var"> | ikw<"calc"> |
             ikw<"url">> {};

  struct ComponentValueAlt;

  // Commas, slashes and equals are values too: retaining them preserves
  // font shorthands, function argument boundaries and custom property text.
  struct VALUE_SEP : terminal<lit<","> | lit<"/"> | lit<"=">> {};

  struct NumberLitRule
      : rule<option(flag<&ast::NumberLit::negative>(ref<MINUS>) |
                    ref<PLUS>) +
             assign<&ast::NumberLit::magnitude>(ref<NUMBER>)> {};

  struct PercentageLitRule
      : rule<option(flag<&ast::PercentageLit::negative>(ref<MINUS>) |
                    ref<PLUS>) +
             assign<&ast::PercentageLit::text>(ref<PERCENTAGE>)> {};

  struct DimensionLitRule
      : rule<option(flag<&ast::DimensionLit::negative>(ref<MINUS>) |
                    ref<PLUS>) +
             assign<&ast::DimensionLit::text>(ref<DIMENSION>)> {};

  struct UnknownDimensionLitRule
      : rule<option(
                 flag<&ast::UnknownDimensionLit::negative>(ref<MINUS>) |
                 ref<PLUS>) +
             assign<&ast::UnknownDimensionLit::text>(
                 ref<UNKNOWN_DIMENSION>)> {};

  struct StringLitRule
      : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};

  struct IdentValueRule : rule<assign<&ast::IdentValue::name>(ref<IdentLikeRule>)> {};

  struct UnicodeRangeLitRule
      : rule<assign<&ast::UnicodeRangeLit::text>(ref<UNICODE_RANGE>)> {};

  struct HashRefRule : rule<assign<&ast::HashRef::text>(ref<HASH>)> {};

  struct UrlRefRule
      : rule<ikw<"url"> + lit<"("> +
             assign<&ast::UrlRef::value>(ref<STRING> | ref<URL_RAW>) +
             lit<")">> {};

  struct VarRefRule
      : rule<ikw<"var"> + lit<"("> +
             assign<&ast::VarRef::name>(ref<IdentLikeRule>) +
             option(flag<&ast::VarRef::hasFallback>(lit<",">) +
                    many(append<&ast::VarRef::fallback>(ref<ComponentValueAlt>))) +
             lit<")">> {};

  struct CalcSumRule;

  struct CalcParenRule
      : rule<lit<"("> + assign<&ast::CalcExpr::sum>(ref<CalcSumRule>) +
             lit<")">> {};

  static constexpr auto CalcValueAlt =
      ref<DimensionLitRule> | ref<UnknownDimensionLitRule> |
      ref<PercentageLitRule> | ref<NumberLitRule> | ref<CalcParenRule>;

  struct CalcProductRule
      : rule<create<ast::CalcProduct> +
             append<&ast::CalcProduct::factors>(CalcValueAlt) +
             many(
                 append<&ast::CalcProduct::ops>(ref<CALC_MUL> | ref<CALC_DIV>) +
                 append<&ast::CalcProduct::factors>(CalcValueAlt))> {};

  // CSS requires whitespace on both sides of a binary `+`/`-` inside
  // calc() so `2px -3px` (a sign, not a subtraction) stays unambiguous --
  // a literal Space token would already have been consumed by a preceding
  // rule's own trivia handling, so `gap::has` reads the raw gap directly
  // instead.
  struct CalcSumRule
      : rule<create<ast::CalcSum> +
             append<&ast::CalcSum::terms>(ref<CalcProductRule>) +
             many(gap::has(SpaceByte) +
                  append<&ast::CalcSum::ops>(ref<CALC_ADD> | ref<CALC_SUB>) +
                  gap::has(SpaceByte) +
                  append<&ast::CalcSum::terms>(ref<CalcProductRule>))> {};

  struct CalcExprRule
      : rule<ikw<"calc"> + lit<"("> +
             assign<&ast::CalcExpr::sum>(ref<CalcSumRule>) + lit<")">> {};

  // A function's name and its `(` must be adjacent -- `rgba (0,0,0,.5)`
  // (with a space) is not a function call in CSS, it is a name followed by
  // a parenthesized group. `gap::none` enforces that the ordinary trivia
  // skip between tokens would otherwise hide.
  // An argument may itself be several juxtaposed values (a gradient
  // color-stop is "color percentage", one comma-separated argument) --
  // Arguments remain an ordered component stream with explicit punctuation,
  // matching Declaration::value.
  struct FunctionCallRule
      : rule<assign<&ast::FunctionCall::name>(ref<IdentLikeRule>) + gap::none +
             lit<"("> +
             option(some(append<&ast::FunctionCall::args>(
                             ref<ComponentValueAlt>))) +
             lit<")">> {};

  struct DxImageTransformRule
      : rule<lit<"progid:DXImageTransform.Microsoft."> +
             assign<&ast::FunctionCall::name>(ref<IDENT>) + gap::none +
             lit<"("> +
             option(some(
                 append<&ast::FunctionCall::args>(
                             ref<ComponentValueAlt>))) +
             lit<")">> {};

  struct PunctValueRule
      : rule<assign<&ast::PunctValue::text>(
            ref<COLON_TOK> | ref<INCLUDES_TXT> | ref<DASHMATCH_TXT> |
            ref<PLUS> | ref<MINUS> | ref<VALUE_SEP> | lit<"."> | lit<">"> |
            lit<"~"> | lit<"|"> | lit<"*"> | lit<"&">)> {};

  struct GroupValueRule
      : rule<create<ast::GroupValue> +
             ((lit<"("> +
               many(append<&ast::GroupValue::items>(ref<ComponentValueAlt>)) +
               lit<")">) |
              (flag<&ast::GroupValue::bracketed>(lit<"[">) +
               many(append<&ast::GroupValue::items>(ref<ComponentValueAlt>)) +
               lit<"]">))> {};

  struct ComponentValueAlt
      : rule<ref<NumberLitRule> | ref<PercentageLitRule> |
             ref<DimensionLitRule> | ref<UnknownDimensionLitRule> |
             ref<StringLitRule> | ref<UnicodeRangeLitRule> |
             ref<UrlRefRule> | ref<VarRefRule> | ref<CalcExprRule> |
             ref<FunctionCallRule> | ref<DxImageTransformRule> |
             ref<HashRefRule> | ref<GroupValueRule> | ref<PunctValueRule> |
             ref<IdentValueRule>> {};

  // The two readings must be made disjoint AT THE FIRST TOKEN, not left for
  // the engine to referee: an ident-led prelude followed by
  // `many(ComponentValueAlt)` can read the SAME span as a real selector
  // (`a` `:` `:` `before`, exactly as `simpleSelectorSequence` does), so for
  // any selector with a pseudo BOTH readings complete and which one wins is
  // not reliable (`a:hover` took the selector, `a::before` did not). None
  // of `simpleSelectorSequence`'s own first tokens -- ident, Hash, `:`,
  // `[`, `*`, `|` -- may start this head; a number, a string, a function
  // token, `(` and a handful of bare punctuation cannot start a selector
  // at all, so they carry no such risk.
  struct HeadPunctValueRule
      : rule<assign<&ast::PunctValue::text>(
            ref<INCLUDES_TXT> | ref<DASHMATCH_TXT> | ref<PLUS> |
            ref<MINUS>)> {};

  struct ParenGroupHeadRule
      : rule<create<ast::GroupValue> + lit<"("> +
             many(append<&ast::GroupValue::items>(ref<ComponentValueAlt>)) +
             lit<")">> {};

  struct UnknownRulesetHeadAlt
      : rule<ref<NumberLitRule> | ref<PercentageLitRule> |
             ref<DimensionLitRule> | ref<UnknownDimensionLitRule> |
             ref<StringLitRule> | ref<UnicodeRangeLitRule> |
             ref<UrlRefRule> | ref<VarRefRule> | ref<CalcExprRule> |
             ref<FunctionCallRule> | ref<DxImageTransformRule> |
             ref<ParenGroupHeadRule> | ref<HeadPunctValueRule>> {};

  // A colon with a real gap after it can never be a pseudo's (see
  // PseudoSelectorRule's `gap::none`), so it is unambiguously the
  // fallback's own bare `:` -- `!gap::none` is the other half of that same
  // fact, used only here.
  struct BareColonRule : rule<assign<&ast::PunctValue::text>(ref<COLON_TOK>)> {
  };

  // Splices into DeclarationRule's own node rather than building its own
  // (a helper writing the enclosing node cannot be its own `rule<>`).
  static constexpr auto PropertyClause =
      (flag<&ast::Declaration::iePrefixStar>(ref<STAR>) +
       assign<&ast::Declaration::property>(ref<IDENT>)) |
      assign<&ast::Declaration::property>(ref<IdentLikeRule>);

  struct DeclarationRule
      : rule<PropertyClause + lit<":"> +
             many(append<&ast::Declaration::value>(ref<ComponentValueAlt>)) +
             option(flag<&ast::Declaration::important>(
                 ref<IMPORTANT>))> {};

  struct TypeRefRule
      : rule<option(option(assign<&ast::TypeRef::nsPrefix>(ref<IdentLikeRule>) |
                           assign<&ast::TypeRef::nsPrefix>(ref<STAR>)) +
                    flag<&ast::TypeRef::hasNsPrefix>(ref<PIPE>)) +
             (flag<&ast::TypeRef::universal>(ref<STAR>) |
              assign<&ast::TypeRef::name>(ref<IdentLikeRule>))> {};

  struct ClassSelectorRule
      : rule<lit<"."> + assign<&ast::ClassSelector::name>(ref<IdentLikeRule>)> {};

  struct IdSelectorRule
      : rule<assign<&ast::IdSelector::name>(ref<HASH>)> {};

  struct AttribSelectorRule
      : rule<lit<"["> +
             option(option(assign<&ast::AttribSelector::nsPrefix>(
                                ref<IdentLikeRule>) |
                           assign<&ast::AttribSelector::nsPrefix>(
                               ref<STAR>)) +
                    flag<&ast::AttribSelector::hasNsPrefix>(ref<PIPE>)) +
             assign<&ast::AttribSelector::name>(ref<IdentLikeRule>) +
             option(assign<&ast::AttribSelector::op>(AttribOpAlt) +
                    assign<&ast::AttribSelector::value>(ref<IdentLikeRule> |
                                                         ref<STRING>) +
                    option(assign<&ast::AttribSelector::modifier>(ref<IdentLikeRule>))) +
             lit<"]">> {};

  // A pseudo's colon(s) are always adjacent to what follows (CSS has no
  // `Space` token there): `gap::none` makes that a grammar fact rather than
  // an accident of trivia, so a colon WITH a following gap can never be a
  // pseudo -- see UnknownRulesetRule's `BareColonRule` alternative.
  struct PseudoSelectorRule
      : rule<lit<":"> + gap::none +
             option(flag<&ast::PseudoSelector::isElement>(lit<":">) +
                    gap::none) +
             ((assign<&ast::PseudoSelector::name>(ref<IdentLikeRule>) + gap::none +
               lit<"("> +
               some(append<&ast::PseudoSelector::args>(
                   ref<ComponentValueAlt>)) +
               lit<")">) |
              assign<&ast::PseudoSelector::name>(ref<IdentLikeRule>))> {};

  static constexpr auto SelectorQualifier =
      append<&ast::SimpleSelectorSequence::items>(
          ref<IdSelectorRule> | ref<ClassSelectorRule> |
          ref<AttribSelectorRule> | ref<PseudoSelectorRule>);

  // A qualifier right after the type/previous qualifier (`div.foo`, a
  // compound selector) means AND; one after a real gap (`div .foo`) starts
  // a new sequence instead -- `gap::none` is what tells them apart, since
  // ordinary trivia skipping would otherwise make both look identical.
  struct SimpleSelectorSequenceRule
      : rule<option(assign<&ast::SimpleSelectorSequence::combinator>(
                 ref<COMBINE_CHILD> | ref<COMBINE_ADJ> | ref<COMBINE_SIB>)) +
             ((assign<&ast::SimpleSelectorSequence::type>(ref<TypeRefRule>) +
               many(gap::none + SelectorQualifier)) |
              (SelectorQualifier + many(gap::none + SelectorQualifier)))> {};

  struct SelectorRule
      : rule<create<ast::Selector> +
             some(append<&ast::Selector::sequences>(
                 ref<SimpleSelectorSequenceRule>))> {};

  struct SelectorGroupRule
      : rule<create<ast::SelectorGroup> +
             some(append<&ast::SelectorGroup::selectors>(ref<SelectorRule>),
                  lit<",">)> {};

  struct RulesetRule
      : rule<assign<&ast::Ruleset::selectors>(ref<SelectorGroupRule>) +
             lit<"{"> +
             many(append<&ast::Ruleset::declarations>(ref<DeclarationRule>) |
                  lit<";">) +
             lit<"}">> {};

  struct UnknownRulesetRule
      : rule<option(
                 (append<&ast::UnknownRuleset::prelude>(
                      ref<UnknownRulesetHeadAlt>) |
                  (append<&ast::UnknownRuleset::prelude>(ref<BareColonRule>) +
                   !gap::none)) +
                 many(append<&ast::UnknownRuleset::prelude>(
                     ref<ComponentValueAlt>))) +
             lit<"{"> +
             many(append<&ast::UnknownRuleset::declarations>(
                      ref<DeclarationRule>) |
                  lit<";">) +
             lit<"}">> {};

  struct MediaExpressionRule
      : rule<lit<"("> + assign<&ast::MediaExpression::feature>(ref<IdentLikeRule>) +
             option(lit<":"> +
                    some(append<&ast::MediaExpression::value>(
                                        ref<ComponentValueAlt>))) +
             lit<")">> {};

  struct MediaQueryRule
      : rule<(option(flag<&ast::MediaQuery::isOnly>(ikw<"only">) |
                     flag<&ast::MediaQuery::isNot>(ikw<"not">)) +
              assign<&ast::MediaQuery::mediaType>(ref<IdentLikeRule>) +
              many(ikw<"and"> + append<&ast::MediaQuery::expressions>(
                                     ref<MediaExpressionRule>))) |
             (append<&ast::MediaQuery::expressions>(ref<MediaExpressionRule>) +
              many(ikw<"and"> + append<&ast::MediaQuery::expressions>(
                                     ref<MediaExpressionRule>)))> {};

  struct MediaQueryListRule
      : rule<create<ast::MediaQueryList> +
             some(append<&ast::MediaQueryList::queries>(ref<MediaQueryRule>),
                  lit<",">)> {};

  struct StatementAlt;

  struct AtMediaRule
      : rule<ikw<"@media"> +
             option(assign<&ast::AtMedia::queries>(ref<MediaQueryListRule>)) +
             lit<"{"> +
             many(append<&ast::AtMedia::body>(ref<StatementAlt>)) +
             lit<"}">> {};

  struct AtPageRule
      : rule<ikw<"@page"> +
             option(lit<":"> + assign<&ast::AtPage::pseudo>(ref<IdentLikeRule>)) +
             lit<"{"> +
             many(append<&ast::AtPage::declarations>(ref<DeclarationRule>) |
                  lit<";">) +
             lit<"}">> {};

  struct AtFontFaceRule
      : rule<ikw<"@font-face"> + lit<"{"> +
             many(
                 append<&ast::AtFontFace::declarations>(ref<DeclarationRule>) |
                 lit<";">) +
             lit<"}">> {};

  static constexpr auto KeyframeStop =
      ikw<"from"> | ikw<"to"> | ref<PERCENTAGE>;

  struct KeyframeSelectorRule
      : rule<create<ast::KeyframeSelector> +
             some(append<&ast::KeyframeSelector::stops>(KeyframeStop),
                  lit<",">)> {};

  struct KeyframeBlockRule
      : rule<assign<&ast::KeyframeBlock::selector>(
                 ref<KeyframeSelectorRule>) +
             lit<"{"> +
             many(append<&ast::KeyframeBlock::declarations>(
                      ref<DeclarationRule>) |
                  lit<";">) +
             lit<"}">> {};

  static constexpr auto KeyframesKw =
      ikw<"@keyframes"> | ikw<"@-moz-keyframes"> | ikw<"@-webkit-keyframes"> |
      ikw<"@-o-keyframes">;

  struct AtKeyframesRule
      : rule<KeyframesKw + assign<&ast::AtKeyframes::name>(ref<IdentLikeRule>) +
             lit<"{"> +
             many(append<&ast::AtKeyframes::blocks>(ref<KeyframeBlockRule>)) +
             lit<"}">> {};

  struct SupportsConditionAlt;

  struct SupportsDeclarationRule
      : rule<lit<"("> +
             assign<&ast::SupportsDeclaration::declaration>(
                 ref<DeclarationRule>) +
             lit<")">> {};

  struct SupportsGeneralRule
      : rule<create<ast::SupportsGeneral> + lit<"("> +
             many(append<&ast::SupportsGeneral::items>(ref<ComponentValueAlt>)) +
             lit<")">> {};

  struct SupportsParenRule
      : rule<lit<"("> +
             assign<&ast::SupportsParen::inner>(ref<SupportsConditionAlt>) +
             lit<")">> {};

  static constexpr auto SupportsConditionInParens =
      ref<SupportsParenRule> | ref<SupportsDeclarationRule> |
      ref<SupportsGeneralRule>;

  // `not (...)` needs no gap:: fix: nothing here demands a mandatory Space
  // that ordinary trivia would already have consumed, so trivia alone
  // already separates `not` from `(`.
  struct SupportsNotRule
      : rule<ikw<"not"> +
             assign<&ast::SupportsNot::operand>(ref<SupportsConditionAlt>)> {
  };

  struct SupportsAndRule
      : rule<create<ast::SupportsAnd> +
             append<&ast::SupportsAnd::operands>(SupportsConditionInParens) +
             some(ikw<"and"> + append<&ast::SupportsAnd::operands>(
                                    SupportsConditionInParens))> {};

  struct SupportsOrRule
      : rule<create<ast::SupportsOr> +
             append<&ast::SupportsOr::operands>(SupportsConditionInParens) +
             some(ikw<"or"> + append<&ast::SupportsOr::operands>(
                                   SupportsConditionInParens))> {};

  struct SupportsConditionAlt
      : rule<ref<SupportsNotRule> | ref<SupportsAndRule> |
             ref<SupportsOrRule> | ref<SupportsParenRule> |
             ref<SupportsDeclarationRule> | ref<SupportsGeneralRule>> {};

  struct AtSupportsRule
      : rule<ikw<"@supports"> +
             assign<&ast::AtSupports::condition>(ref<SupportsConditionAlt>) +
             lit<"{"> +
             many(append<&ast::AtSupports::body>(ref<StatementAlt>)) +
             lit<"}">> {};

  struct AtViewportRule
      : rule<ikw<"@viewport"> + lit<"{"> +
             many(
                 append<&ast::AtViewport::declarations>(ref<DeclarationRule>) |
                 lit<";">) +
             lit<"}">> {};

  struct AtCounterStyleRule
      : rule<ikw<"@counter-style"> +
             assign<&ast::AtCounterStyle::name>(ref<IdentLikeRule>) + lit<"{"> +
             many(append<&ast::AtCounterStyle::declarations>(
                      ref<DeclarationRule>) |
                  lit<";">) +
             lit<"}">> {};

  struct FeatureValueDefRule
      : rule<assign<&ast::FeatureValueDef::name>(ref<IdentLikeRule>) + lit<":"> +
             some(append<&ast::FeatureValueDef::values>(
                 ref<NumberLitRule>))> {};

  struct FeatureValueBlockRule
      : rule<assign<&ast::FeatureValueBlock::type>(ref<AT_KEYWORD>) +
             lit<"{"> +
             many(append<&ast::FeatureValueBlock::defs>(
                      ref<FeatureValueDefRule>) |
                  lit<";">) +
             lit<"}">> {};

  struct FontFamilyNameRule
      : rule<create<ast::FontFamilyName> +
             (append<&ast::FontFamilyName::words>(ref<STRING>) |
              some(append<&ast::FontFamilyName::words>(ref<IdentLikeRule>)))> {};

  struct AtFontFeatureValuesRule
      : rule<ikw<"@font-feature-values"> +
             some(append<&ast::AtFontFeatureValues::familyNames>(
                      ref<FontFamilyNameRule>),
                  lit<",">) +
             lit<"{"> +
             many(append<&ast::AtFontFeatureValues::blocks>(
                 ref<FeatureValueBlockRule>)) +
             lit<"}">> {};

  struct AtCharsetRule
      : rule<ikw<"@charset"> + assign<&ast::AtCharset::value>(ref<STRING>) +
             option(lit<";">)> {};

  static constexpr auto ImportUrlClause =
      assign<&ast::AtImport::url>(ref<STRING>) |
      (ikw<"url"> + lit<"("> +
       assign<&ast::AtImport::url>(ref<STRING> | ref<URL_RAW>) + lit<")">);

  struct AtImportRule
      : rule<ikw<"@import"> + ImportUrlClause +
             option(assign<&ast::AtImport::media>(ref<MediaQueryListRule>)) +
             option(lit<";">)> {};

  static constexpr auto NamespaceUrlClause =
      assign<&ast::AtNamespace::url>(ref<STRING>) |
      (ikw<"url"> + lit<"("> +
       assign<&ast::AtNamespace::url>(ref<STRING> | ref<URL_RAW>) +
       lit<")">);

  struct AtNamespaceRule
      : rule<ikw<"@namespace"> +
             option(assign<&ast::AtNamespace::prefix>(ref<IdentLikeRule>)) +
             NamespaceUrlClause + option(lit<";">)> {};

  // The rare fallback for content nested inside an unrecognized at-rule or a
  // malformed declaration value -- kept flat
  // rather than as separately ordered runs of declarations, nested
  // statements and bare tokens.
  struct RawBlockRule
      : rule<create<ast::RawBlock> + lit<"{"> +
             many(append<&ast::RawBlock::items>(ref<DeclarationRule>) |
                  append<&ast::RawBlock::items>(ref<StatementAlt>) |
                  append<&ast::RawBlock::items>(ref<ComponentValueAlt>) |
                  append<&ast::RawBlock::items>(ref<RawBlockRule>) |
                  lit<";">) +
             lit<"}">> {};

  struct UnknownAtRuleRule
      : rule<assign<&ast::UnknownAtRule::name>(ref<AT_KEYWORD>) +
             many(
                 append<&ast::UnknownAtRule::prelude>(ref<ComponentValueAlt>)) +
             option(assign<&ast::UnknownAtRule::body>(ref<RawBlockRule>)) +
             option(lit<";">)> {};

  struct StatementAlt
      : rule<ref<AtCharsetRule> | ref<AtImportRule> | ref<AtNamespaceRule> |
             ref<AtMediaRule> | ref<AtPageRule> | ref<AtFontFaceRule> |
             ref<AtKeyframesRule> | ref<AtSupportsRule> |
             ref<AtViewportRule> | ref<AtCounterStyleRule> |
             ref<AtFontFeatureValuesRule> | ref<UnknownAtRuleRule> |
             ref<RulesetRule> | ref<UnknownRulesetRule>> {};

  struct StylesheetRule
      : rule<create<ast::Stylesheet> +
             many(append<&ast::Stylesheet::statements>(ref<StatementAlt>))> {
  };

  using entry = StylesheetRule;
  using ignore = list<SPACE, CDO, CDC>;
  using hidden = list<ML_COMMENT>;
};

}  // namespace css3
