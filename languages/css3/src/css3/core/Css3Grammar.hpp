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

inline std::string_view decode_css(std::string_view body,
                                   std::pmr::memory_resource* mem) {
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
}

inline constexpr auto unquote = [](std::string_view text,
                                   std::pmr::memory_resource* mem) {
  return decode_css(text.substr(1, text.size() - 2), mem);
};

struct Css3 : cursiv::GrammarSpec {
  // `-` is a legal identifier byte throughout CSS (`font-size`), so the
  // keyword boundary must include it or `kw<"only">` would run into `-only`.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_-"> |
      cr<R"(\u{80}-\u{10FFFF})"> | cr<R"(\\)">;

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
  struct NUMBER : terminal<as<double>(NumberBody)> {};
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
  struct STRING : terminal<as<unquote>((lit<"\""> + StringBody1 + lit<"\"">) |
                                       (lit<"'"> + StringBody2 + lit<"'">))> {};

  template<unsigned HexCount>
  static constexpr auto WildcardRange = repeat<HexCount>(HexDigit) +
      repeat<1, 6 - HexCount>(lit<"?">);
  // Question marks follow the hex prefix; a wildcard range has no `-end`.
  struct UNICODE_RANGE
      : terminal<cr<"uU"> + lit<"+"> +
                 (repeat<1, 6>(lit<"?">) | WildcardRange<1> | WildcardRange<2> |
                  WildcardRange<3> | WildcardRange<4> | WildcardRange<5> |
                  (repeat<1, 6>(HexDigit) + option(lit<"-"> + repeat<1, 6>(HexDigit))))> {};

  static constexpr auto UrlRawByte =
      ~cr<R"(\x00- "'-)\\\x7f)"> | IdentEscape;
  struct URL_RAW : terminal<as<decode_css>(some(UrlRawByte))> {};

  struct IMPORTANT
      : terminal<lit<"!"> +
                 many(cr<R"( \t\r\n\f)"> | through(lit<"/*">, lit<"*/">)) +
                 ikw<"important">> {};

  static constexpr auto SpaceByte = cr<R"( \t\r\n\f)">;
  // Comments do not create a descendant combinator or calc whitespace.
  // Spell the regular comment pattern here because gap conditions cannot
  // call a terminal or use through().
  static constexpr auto CommentGap = lit<"/*"> +
      many(cr<"^*"> | (some(lit<"*">) + cr<"^*/">)) + some(lit<"*">) + lit<"/">;
  static constexpr auto SelectorTight = gap::is(many(CommentGap));
  static constexpr auto CssWhitespaceGap = gap::is(
      many(CommentGap) + some(SpaceByte) + many(SpaceByte | CommentGap));

  struct STAR : terminal<lit<"*">> {};
  struct PIPE : terminal<lit<"|">> {};
  struct MINUS : terminal<lit<"-">> {};
  struct PLUS : terminal<lit<"+">> {};

  struct INCLUDES : terminal<as<ast::AttribOp::includes>(lit<"~=">)> {};
  struct DASHMATCH : terminal<as<ast::AttribOp::dashmatch>(lit<"|=">)> {};
  static constexpr auto AttribOpAlt = as<ast::AttribOp::prefix>(lit<"^=">) | as<ast::AttribOp::suffix>(lit<"$=">) |
                                       as<ast::AttribOp::substring>(lit<"*=">) | ref<INCLUDES> |
                                       ref<DASHMATCH> | as<ast::AttribOp::equals>(lit<"=">);


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
  // `var`/`calc`/`url` are ALSO fixed introducers elsewhere (VarRef,
  // CalcExpr, UrlRef, each `ikw<"...."> + lit<"(">`) -- spelled
  // once here too, or the state offering both readings would commit to the
  // keyword and never fall back to a plain name when no `(` follows.
  struct IdentLike
      : rule<ref<IDENT> | ikw<"only"> | ikw<"not"> | ikw<"and"> | ikw<"or"> |
             ikw<"from"> | ikw<"to"> | ikw<"var"> | ikw<"calc"> |
             ikw<"url">> {};

  struct ComponentValueAlt;
  struct RawBlock;

  // Commas, slashes and equals are values too: retaining them preserves
  // font shorthands, function argument boundaries and custom property text.
  struct VALUE_SEP : terminal<lit<","> | lit<"/"> | lit<"=">> {};

  struct NumberLit
      : rule<option((flag<&ast::NumberLit::negative>(ref<MINUS>) |
                     ref<PLUS>) + gap::none) +
             assign<&ast::NumberLit::magnitude>(ref<NUMBER>)> {};

  struct PercentageLit
      : rule<option((flag<&ast::PercentageLit::negative>(ref<MINUS>) |
                     ref<PLUS>) + gap::none) +
             assign<&ast::PercentageLit::text>(ref<PERCENTAGE>)> {};

  struct DimensionLit
      : rule<option((flag<&ast::DimensionLit::negative>(ref<MINUS>) |
                     ref<PLUS>) + gap::none) +
             assign<&ast::DimensionLit::text>(ref<DIMENSION>)> {};

  struct UnknownDimensionLit
      : rule<option((
                 flag<&ast::UnknownDimensionLit::negative>(ref<MINUS>) |
                 ref<PLUS>) + gap::none) +
             assign<&ast::UnknownDimensionLit::text>(
                 ref<UNKNOWN_DIMENSION>)> {};

  struct StringLit
      : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};

  struct IdentValue : rule<assign<&ast::IdentValue::name>(ref<IdentLike>)> {};

  struct UnicodeRangeLit
      : rule<assign<&ast::UnicodeRangeLit::text>(ref<UNICODE_RANGE>)> {};

  struct HashRef : rule<assign<&ast::HashRef::text>(ref<HASH>)> {};

  struct UrlRef
      : rule<ikw<"url"> + gap::none + lit<"("> +
             option(assign<&ast::UrlRef::value>(ref<STRING> | ref<URL_RAW>)) +
             lit<")">> {};

  struct VarRef
      : rule<ikw<"var"> + gap::none + lit<"("> +
             assign<&ast::VarRef::name>(ref<IdentLike>) +
             option(flag<&ast::VarRef::hasFallback>(lit<",">) +
                    many(append<&ast::VarRef::fallback>(ref<ComponentValueAlt>))) +
             lit<")">> {};

  struct CalcSum;

  struct CalcParen
      : rule<lit<"("> + assign<&ast::CalcExpr::sum>(ref<CalcSum>) +
             lit<")">> {};

  static constexpr auto CalcValueAlt =
      ref<DimensionLit> | ref<UnknownDimensionLit> |
      ref<PercentageLit> | ref<NumberLit> | ref<CalcParen>;

  struct CalcProduct
      : rule<create<ast::CalcProduct> +
             append<&ast::CalcProduct::factors>(CalcValueAlt) +
             many(
                 append<&ast::CalcProduct::ops>(as<ast::CalcOp::multiply>(lit<"*">) | as<ast::CalcOp::divide>(lit<"/">)) +
                 append<&ast::CalcProduct::factors>(CalcValueAlt))> {};

  // CSS requires whitespace on both sides of a binary `+`/`-` inside
  // calc() so `2px -3px` (a sign, not a subtraction) stays unambiguous --
  // trivia skipping would otherwise hide it; the gap pattern counts only
  // whitespace outside comments.
  struct CalcSum
      : rule<create<ast::CalcSum> +
             append<&ast::CalcSum::terms>(ref<CalcProduct>) +
             many(CssWhitespaceGap +
                  append<&ast::CalcSum::ops>(as<ast::CalcOp::add>(lit<"+">) | as<ast::CalcOp::subtract>(lit<"-">)) +
                  CssWhitespaceGap +
                  append<&ast::CalcSum::terms>(ref<CalcProduct>))> {};

  struct CalcExpr
      : rule<ikw<"calc"> + gap::none + lit<"("> +
             assign<&ast::CalcExpr::sum>(ref<CalcSum>) + lit<")">> {};

  // A function's name and its `(` must be adjacent -- `rgba (0,0,0,.5)`
  // (with a space) is not a function call in CSS, it is a name followed by
  // a parenthesized group. `gap::none` enforces that the ordinary trivia
  // skip between tokens would otherwise hide.
  // An argument may itself be several juxtaposed values (a gradient
  // color-stop is "color percentage", one comma-separated argument) --
  // Arguments remain an ordered component stream with explicit punctuation,
  // matching Declaration::value.
  struct FunctionCall
      : rule<assign<&ast::FunctionCall::name>(ref<IdentLike>) + gap::none +
             lit<"("> +
             option(some(append<&ast::FunctionCall::args>(
                             ref<ComponentValueAlt>))) +
             lit<")">> {};

  struct DxImageTransform
      : rule<lit<"progid:DXImageTransform.Microsoft."> +
             assign<&ast::FunctionCall::name>(ref<IDENT>) + gap::none +
             lit<"("> +
             option(some(
                 append<&ast::FunctionCall::args>(
                             ref<ComponentValueAlt>))) +
             lit<")">> {};

  struct PunctValue
      : rule<assign<&ast::PunctValue::text>(
            ref<COLON_TOK> | ref<INCLUDES_TXT> | ref<DASHMATCH_TXT> |
            ref<PLUS> | ref<MINUS> | ref<VALUE_SEP> | lit<"."> | lit<">"> |
            lit<"~"> | lit<"|"> | lit<"*"> | lit<"&">)> {};

  struct GroupValue
      : rule<create<ast::GroupValue> +
             ((lit<"("> +
               many(append<&ast::GroupValue::items>(ref<ComponentValueAlt>)) +
               lit<")">) |
              (flag<&ast::GroupValue::bracketed>(lit<"[">) +
               many(append<&ast::GroupValue::items>(ref<ComponentValueAlt>)) +
               lit<"]">))> {};

  struct ComponentValueAlt
      : rule<ref<NumberLit> | ref<PercentageLit> |
             ref<DimensionLit> | ref<UnknownDimensionLit> |
             ref<StringLit> | ref<UnicodeRangeLit> |
             ref<UrlRef> | ref<VarRef> | ref<CalcExpr> |
             ref<FunctionCall> | ref<DxImageTransform> |
             ref<HashRef> | ref<GroupValue> | ref<RawBlock> | ref<PunctValue> |
             ref<IdentValue>> {};

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
  struct HeadPunctValue
      : rule<assign<&ast::PunctValue::text>(
            ref<INCLUDES_TXT> | ref<DASHMATCH_TXT> | ref<PLUS> |
            ref<MINUS>)> {};

  struct ParenGroupHead
      : rule<create<ast::GroupValue> + lit<"("> +
             many(append<&ast::GroupValue::items>(ref<ComponentValueAlt>)) +
             lit<")">> {};

  struct UnknownRulesetHeadAlt
      : rule<ref<NumberLit> | ref<PercentageLit> |
             ref<DimensionLit> | ref<UnknownDimensionLit> |
             ref<StringLit> | ref<UnicodeRangeLit> |
             ref<UrlRef> | ref<VarRef> | ref<CalcExpr> |
             ref<FunctionCall> | ref<DxImageTransform> |
             ref<ParenGroupHead> | ref<HeadPunctValue>> {};

  // Real whitespace after a colon excludes a pseudo; comment-only gaps
  // still belong to PseudoSelector. Keep the fallback disjoint.
  struct BareColon : rule<assign<&ast::PunctValue::text>(ref<COLON_TOK>)> {
  };

  // Splices into DeclarationRule's own node rather than building its own
  // (a helper writing the enclosing node cannot be its own `rule<>`).
  static constexpr auto PropertyClause =
      (flag<&ast::Declaration::iePrefixStar>(ref<STAR>) +
       assign<&ast::Declaration::property>(ref<IDENT>)) |
      assign<&ast::Declaration::property>(ref<IdentLike>);

  struct DeclarationRule
      : rule<PropertyClause + lit<":"> +
             many(append<&ast::Declaration::value>(ref<ComponentValueAlt>)) +
             option(flag<&ast::Declaration::important>(
                 ref<IMPORTANT>))> {};

  struct TypeRef
      : rule<option(option(assign<&ast::TypeRef::nsPrefix>(ref<IdentLike>) |
                           assign<&ast::TypeRef::nsPrefix>(ref<STAR>)) +
                    flag<&ast::TypeRef::hasNsPrefix>(ref<PIPE>)) +
             (flag<&ast::TypeRef::universal>(ref<STAR>) |
              assign<&ast::TypeRef::name>(ref<IdentLike>))> {};

  struct ClassSelector
      : rule<lit<"."> + SelectorTight + assign<&ast::ClassSelector::name>(ref<IdentLike>)> {};

  struct IdSelector
      : rule<assign<&ast::IdSelector::name>(ref<HASH>)> {};

  struct AttribSelector
      : rule<lit<"["> +
             option(option(assign<&ast::AttribSelector::nsPrefix>(
                                ref<IdentLike>) |
                           assign<&ast::AttribSelector::nsPrefix>(
                               ref<STAR>)) +
                    flag<&ast::AttribSelector::hasNsPrefix>(ref<PIPE>)) +
             assign<&ast::AttribSelector::name>(ref<IdentLike>) +
             option(assign<&ast::AttribSelector::op>(AttribOpAlt) +
                    assign<&ast::AttribSelector::value>(ref<IdentLike> |
                                                         ref<STRING>) +
                    option(assign<&ast::AttribSelector::modifier>(ref<IdentLike>))) +
             lit<"]">> {};

  // A pseudo's colon(s) are always adjacent to what follows (CSS has no
  // `Space` token there): `gap::none` makes that a grammar fact rather than
  // an accident of trivia, so a colon WITH a following gap can never be a
  // pseudo -- see UnknownRuleset's `BareColon` alternative.
  struct PseudoSelector
      : rule<lit<":"> + SelectorTight +
             option(flag<&ast::PseudoSelector::isElement>(lit<":">) +
                    SelectorTight) +
             ((assign<&ast::PseudoSelector::name>(ref<IdentLike>) + gap::none +
               lit<"("> +
               some(append<&ast::PseudoSelector::args>(
                   ref<ComponentValueAlt>)) +
               lit<")">) |
              assign<&ast::PseudoSelector::name>(ref<IdentLike>))> {};

  static constexpr auto SelectorQualifier =
      append<&ast::SimpleSelectorSequence::items>(
          ref<IdSelector> | ref<ClassSelector> |
          ref<AttribSelector> | ref<PseudoSelector>);

  // A qualifier right after the type/previous qualifier (`div.foo`, a
  // compound selector) means AND; one after a real gap (`div .foo`) starts
  // a new sequence instead -- `gap::none` is what tells them apart, since
  // ordinary trivia skipping would otherwise make both look identical.
  struct SimpleSelectorSequence
      : rule<option(assign<&ast::SimpleSelectorSequence::combinator>(
                 as<ast::Combinator::child>(lit<">">) | as<ast::Combinator::adjacent_sibling>(lit<"+">) | as<ast::Combinator::general_sibling>(lit<"~">))) +
             ((assign<&ast::SimpleSelectorSequence::type>(ref<TypeRef>) +
               many(SelectorTight + SelectorQualifier)) |
              (SelectorQualifier + many(SelectorTight + SelectorQualifier)))> {};

  struct Selector
      : rule<create<ast::Selector> +
             append<&ast::Selector::sequences>(ref<SimpleSelectorSequence>) +
             many((&(lit<">"> | lit<"+"> | lit<"~">) | CssWhitespaceGap) +
                  append<&ast::Selector::sequences>(ref<SimpleSelectorSequence>))> {};

  struct SelectorGroup
      : rule<create<ast::SelectorGroup> +
             some(append<&ast::SelectorGroup::selectors>(ref<Selector>),
                  lit<",">)> {};

  struct Ruleset
      : rule<assign<&ast::Ruleset::selectors>(ref<SelectorGroup>) +
             lit<"{"> +
             many(append<&ast::Ruleset::declarations>(ref<DeclarationRule>) |
                  lit<";">) +
             lit<"}">> {};

  struct UnknownRuleset
      : rule<option(
                 (append<&ast::UnknownRuleset::prelude>(
                      ref<UnknownRulesetHeadAlt>) |
                  (append<&ast::UnknownRuleset::prelude>(ref<BareColon>) +
                   CssWhitespaceGap)) +
                 many(append<&ast::UnknownRuleset::prelude>(
                     ref<ComponentValueAlt>))) +
             lit<"{"> +
             many(append<&ast::UnknownRuleset::declarations>(
                      ref<DeclarationRule>) |
                  lit<";">) +
             lit<"}">> {};

  struct MediaExpression
      : rule<lit<"("> + assign<&ast::MediaExpression::feature>(ref<IdentLike>) +
             option(lit<":"> +
                    some(append<&ast::MediaExpression::value>(
                                        ref<ComponentValueAlt>))) +
             lit<")">> {};

  struct MediaQuery
      : rule<(option(flag<&ast::MediaQuery::isOnly>(ikw<"only">) |
                     flag<&ast::MediaQuery::isNot>(ikw<"not">)) +
              assign<&ast::MediaQuery::mediaType>(ref<IdentLike>) +
              many(ikw<"and"> + append<&ast::MediaQuery::expressions>(
                                     ref<MediaExpression>))) |
             (append<&ast::MediaQuery::expressions>(ref<MediaExpression>) +
              many(ikw<"and"> + append<&ast::MediaQuery::expressions>(
                                     ref<MediaExpression>)))> {};

  struct MediaQueryList
      : rule<create<ast::MediaQueryList> +
             some(append<&ast::MediaQueryList::queries>(ref<MediaQuery>),
                  lit<",">)> {};

  struct StatementAlt;

  struct AtMedia
      : rule<ikw<"@media"> +
             option(assign<&ast::AtMedia::queries>(ref<MediaQueryList>)) +
             lit<"{"> +
             many(append<&ast::AtMedia::body>(ref<StatementAlt>)) +
             lit<"}">> {};

  static constexpr auto PagePseudo = lit<":"> + SelectorTight +
      append<&ast::PageSelector::pseudos>(ref<IdentLike>);
  struct PageSelector
      : rule<(assign<&ast::PageSelector::name>(ref<IdentLike>) +
              many(SelectorTight + PagePseudo)) |
             (PagePseudo + many(SelectorTight + PagePseudo))> {};

  struct PageMargin
      : rule<assign<&ast::PageMargin::name>(ref<AT_KEYWORD>) + lit<"{"> +
             many(append<&ast::PageMargin::declarations>(ref<DeclarationRule>) |
                  lit<";">) + lit<"}">> {};

  struct AtPage
      : rule<ikw<"@page"> +
             option(some(append<&ast::AtPage::selectors>(ref<PageSelector>), lit<",">)) +
             lit<"{"> +
             many(append<&ast::AtPage::declarations>(ref<DeclarationRule>) |
                  append<&ast::AtPage::margins>(ref<PageMargin>) |
                  lit<";">) +
             lit<"}">> {};

  struct AtFontFace
      : rule<ikw<"@font-face"> + lit<"{"> +
             many(
                 append<&ast::AtFontFace::declarations>(ref<DeclarationRule>) |
                 lit<";">) +
             lit<"}">> {};

  static constexpr auto KeyframeStop =
      ikw<"from"> | ikw<"to"> | ref<PERCENTAGE>;

  struct KeyframeSelector
      : rule<create<ast::KeyframeSelector> +
             some(append<&ast::KeyframeSelector::stops>(KeyframeStop),
                  lit<",">)> {};

  struct KeyframeBlock
      : rule<assign<&ast::KeyframeBlock::selector>(
                 ref<KeyframeSelector>) +
             lit<"{"> +
             many(append<&ast::KeyframeBlock::declarations>(
                      ref<DeclarationRule>) |
                  lit<";">) +
             lit<"}">> {};

  static constexpr auto KeyframesKw =
      ikw<"@keyframes"> | ikw<"@-moz-keyframes"> | ikw<"@-webkit-keyframes"> |
      ikw<"@-o-keyframes">;

  struct AtKeyframes
      : rule<KeyframesKw + assign<&ast::AtKeyframes::name>(ref<IdentLike> | ref<STRING>) +
             lit<"{"> +
             many(append<&ast::AtKeyframes::blocks>(ref<KeyframeBlock>)) +
             lit<"}">> {};

  struct SupportsConditionAlt;

  struct SupportsDeclaration
      : rule<lit<"("> +
             assign<&ast::SupportsDeclaration::declaration>(
                 ref<DeclarationRule>) +
             lit<")">> {};

  struct SupportsGeneral
      : rule<create<ast::SupportsGeneral>(
                 append<&ast::SupportsGeneral::items>(ref<FunctionCall>) |
                 (lit<"("> +
                  many(append<&ast::SupportsGeneral::items>(ref<ComponentValueAlt>)) +
                  lit<")">))> {};

  struct SupportsParen
      : rule<lit<"("> +
             assign<&ast::SupportsParen::inner>(ref<SupportsConditionAlt>) +
             lit<")">> {};

  static constexpr auto SupportsConditionInParens =
      ref<SupportsParen> | ref<SupportsDeclaration> |
      ref<SupportsGeneral>;

  // A comment or whitespace separates an identifier token from `(`;
  // adjacent `not(` is a function token, hence general-enclosed instead.
  struct SupportsNot
      : rule<ikw<"not"> + !gap::none +
             assign<&ast::SupportsNot::operand>(SupportsConditionInParens)> {
  };

  struct SupportsAnd
      : rule<create<ast::SupportsAnd> +
             append<&ast::SupportsAnd::operands>(SupportsConditionInParens) +
             some(ikw<"and"> + !gap::none + append<&ast::SupportsAnd::operands>(
                                    SupportsConditionInParens))> {};

  struct SupportsOr
      : rule<create<ast::SupportsOr> +
             append<&ast::SupportsOr::operands>(SupportsConditionInParens) +
             some(ikw<"or"> + !gap::none + append<&ast::SupportsOr::operands>(
                                   SupportsConditionInParens))> {};

  struct SupportsConditionAlt
      : rule<ref<SupportsNot> | ref<SupportsAnd> |
             ref<SupportsOr> | ref<SupportsParen> |
             ref<SupportsDeclaration> | ref<SupportsGeneral>> {};

  struct AtSupports
      : rule<ikw<"@supports"> +
             assign<&ast::AtSupports::condition>(ref<SupportsConditionAlt>) +
             lit<"{"> +
             many(append<&ast::AtSupports::body>(ref<StatementAlt>)) +
             lit<"}">> {};

  struct AtViewport
      : rule<ikw<"@viewport"> + lit<"{"> +
             many(
                 append<&ast::AtViewport::declarations>(ref<DeclarationRule>) |
                 lit<";">) +
             lit<"}">> {};

  struct AtCounterStyle
      : rule<ikw<"@counter-style"> +
             assign<&ast::AtCounterStyle::name>(ref<IdentLike>) + lit<"{"> +
             many(append<&ast::AtCounterStyle::declarations>(
                      ref<DeclarationRule>) |
                  lit<";">) +
             lit<"}">> {};

  struct FeatureValueDef
      : rule<assign<&ast::FeatureValueDef::name>(ref<IdentLike>) + lit<":"> +
             some(append<&ast::FeatureValueDef::values>(
                 ref<NumberLit>))> {};

  struct FeatureValueBlock
      : rule<assign<&ast::FeatureValueBlock::type>(ref<AT_KEYWORD>) +
             lit<"{"> +
             many(append<&ast::FeatureValueBlock::defs>(
                      ref<FeatureValueDef>) |
                  lit<";">) +
             lit<"}">> {};

  struct FontFamilyName
      : rule<create<ast::FontFamilyName> +
             (append<&ast::FontFamilyName::words>(ref<STRING>) |
              some(append<&ast::FontFamilyName::words>(ref<IdentLike>)))> {};

  struct AtFontFeatureValues
      : rule<ikw<"@font-feature-values"> +
             some(append<&ast::AtFontFeatureValues::familyNames>(
                      ref<FontFamilyName>),
                  lit<",">) +
             lit<"{"> +
             many(append<&ast::AtFontFeatureValues::blocks>(
                 ref<FeatureValueBlock>)) +
             lit<"}">> {};

  struct AtCharset
      : rule<ikw<"@charset"> + assign<&ast::AtCharset::value>(ref<STRING>) +
             option(lit<";">)> {};

  static constexpr auto ImportUrlClause =
      assign<&ast::AtImport::url>(ref<STRING>) |
      (ikw<"url"> + gap::none + lit<"("> +
       option(assign<&ast::AtImport::url>(ref<STRING> | ref<URL_RAW>)) + lit<")">);

  struct AtImport
      : rule<ikw<"@import"> + ImportUrlClause +
             option(assign<&ast::AtImport::media>(ref<MediaQueryList>)) +
             option(lit<";">)> {};

  static constexpr auto NamespaceUrlClause =
      assign<&ast::AtNamespace::url>(ref<STRING>) |
      (ikw<"url"> + gap::none + lit<"("> +
       option(assign<&ast::AtNamespace::url>(ref<STRING> | ref<URL_RAW>)) +
       lit<")">);

  struct AtNamespace
      : rule<ikw<"@namespace"> +
             option(assign<&ast::AtNamespace::prefix>(ref<IdentLike>)) +
             NamespaceUrlClause + option(lit<";">)> {};

  // The rare fallback for content nested inside an unrecognized at-rule or a
  // malformed declaration value -- kept flat
  // rather than as separately ordered runs of declarations, nested
  // statements and bare tokens.
  struct RawBlock
      : rule<create<ast::RawBlock> + lit<"{"> +
             many(append<&ast::RawBlock::items>(ref<DeclarationRule>) |
                  append<&ast::RawBlock::items>(ref<StatementAlt>) |
                  append<&ast::RawBlock::items>(ref<ComponentValueAlt>) |
                  lit<";">) +
             lit<"}">> {};

  struct UnknownAtRule
      : rule<assign<&ast::UnknownAtRule::name>(ref<AT_KEYWORD>) +
             many(
                 append<&ast::UnknownAtRule::prelude>(ref<ComponentValueAlt>)) +
             option(assign<&ast::UnknownAtRule::body>(ref<RawBlock>)) +
             option(lit<";">)> {};

  struct StatementAlt
      : rule<ref<AtCharset> | ref<AtImport> | ref<AtNamespace> |
             ref<AtMedia> | ref<AtPage> | ref<AtFontFace> |
             ref<AtKeyframes> | ref<AtSupports> |
             ref<AtViewport> | ref<AtCounterStyle> |
             ref<AtFontFeatureValues> | ref<UnknownAtRule> |
             ref<Ruleset> | ref<UnknownRuleset>> {};

  struct Stylesheet
      : rule<create<ast::Stylesheet> +
             many(append<&ast::Stylesheet::statements>(ref<StatementAlt>))> {
  };

  using entry = Stylesheet;
  using ignored = list<SPACE, CDO, CDC>;
  using hidden = list<ML_COMMENT>;
};

}  // namespace css3
