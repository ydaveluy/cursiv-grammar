#pragma once

#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pcre/core/ast.hpp"

namespace pcre {

inline constexpr auto comment_text = [](std::string_view text) {
  return text.substr(3, text.size() - 4);
};
inline constexpr auto callout_text =
    [](std::string_view text,
       std::pmr::memory_resource *memory) -> std::string_view {
  if (text.size() == 2)
    return text.substr(1, 0);
  auto *data =
      static_cast<char *>(memory->allocate(text.size() - 2, alignof(char)));
  std::size_t size = 0;
  for (std::size_t i = 1; i + 1 < text.size(); ++i) {
    data[size++] = text[i];
    if (text[i] == text.back() && i + 2 < text.size() &&
        text[i + 1] == text.back())
      ++i;
  }
  return {data, size};
};
inline constexpr auto backref_number = [](std::string_view text) {
  std::int64_t value = 0;
  for (char c : text.substr(1)) {
    if (value > (INT64_MAX - (c - '0')) / 10)
      return INT64_MAX;
    value = value * 10 + c - '0';
  }
  return value;
};

struct Pcre : cursiv::GrammarSpec {
  struct Alternation;
  struct AtomRule;
  struct GroupRule;
  struct CharacterClassRule;

  static constexpr auto Digit = cr<"0-9">;
  static constexpr auto Hex = cr<"0-9a-fA-F">;
  static constexpr auto Space = many(cr<" \t">);
  static constexpr auto Name = some(uc::L | uc::Nd | lit<"_">);
  struct NameTerm : terminal<Name> {};
  struct DigitsTerm : terminal<some(Digit), std::int64_t> {};
  struct FlagTerm : terminal<some(cr<"aimnrsxJUDPTSW">)> {};

  struct CommentTerm : terminal<lit<"(?#"> + many(cr<"^)">) + lit<")">,
                                text_conv<comment_text>> {};
  struct CommentRule : rule<assign<&ast::Comment::text>(ref<CommentTerm>)> {};
  struct QuoteBoundaryTerm : terminal<lit<"\\Q\\E"> | lit<"\\E">> {};
  struct QuoteBoundaryRule
      : rule<assign<&ast::QuoteBoundary::text>(ref<QuoteBoundaryTerm>)> {};
  static constexpr auto Annotation = ref<CommentRule> | ref<QuoteBoundaryRule>;

  static constexpr auto Code = lit<"{"> + many(cr<"^}">) + lit<"}">;
  static constexpr auto CharacterEscape =
      cr<"aefnrt"> | (lit<"c"> + any) | (lit<"0"> + repeat<0, 2>(cr<"0-7">)) |
      (lit<"o"> + Code) | (lit<"x"> + (Code | repeat<0, 2>(Hex))) |
      (lit<"N{U+"> + many(cr<"^}">) + lit<"}">) | (lit<"u"> + repeat<4>(Hex));
  struct EscapeLiteralTerm
      : terminal<lit<"\\"> +
                 (CharacterEscape | cr<"^1-9QECdDhHNRsSvVwWXbBAzZGK">)> {};
  struct PlainCharTerm : terminal<cr<R"(^\\.^$()[?*+{|)">> {};
  struct AmbiguousNumberTerm : terminal<lit<"\\"> + cr<"1-7"> + some(Digit)> {};
  struct AmbiguousNumberRule
      : rule<assign<&ast::NumericEscape::text>(ref<AmbiguousNumberTerm>)> {};
  struct BareBackrefTerm
      : terminal<lit<"\\"> + ((cr<"1-7"> + !Digit) | (cr<"89"> + many(Digit))),
                 text_conv<backref_number>> {};

  static constexpr auto CountShape =
      lit<"{"> + Space +
      ((some(Digit) + Space + option(lit<","> + Space + many(Digit) + Space)) |
       (lit<","> + Space + some(Digit) + Space)) +
      lit<"}">;
  struct LiteralAtom
      : rule<assign<&ast::Literal::text>(ref<EscapeLiteralTerm> |
                                         ref<PlainCharTerm>) |
             (!CountShape + assign<&ast::Literal::text>(lit<"{">))> {};

  static constexpr auto TypeEscape =
      cr<"CdDhHNRsSvVwWX"> |
      (cr<"pP"> + ((lit<"{"> + many(cr<"^}">) + lit<"}">) | cr<"A-Za-z">));
  struct TypeTerm : terminal<lit<"\\"> + TypeEscape> {};
  struct CharacterType
      : rule<assign<&ast::CharacterType::text>(lit<"."> | ref<TypeTerm>)> {};
  struct ClassType : rule<assign<&ast::CharacterType::text>(ref<TypeTerm>)> {};
  struct AnchorTerm : terminal<lit<"\\"> + cr<"bBAzZG">> {};
  struct Anchor : rule<assign<&ast::Anchor::text>(ref<AnchorTerm> | lit<"^"> |
                                                  lit<"$">)> {};
  struct MatchPointReset : rule<create<ast::MatchPointReset> + lit<"\\K">> {};

  struct ClassEscapeTerm
      : terminal<lit<"\\"> + (CharacterEscape |
                              (cr<"1-7"> + repeat<0, 2>(cr<"0-7">)) |
                              cr<"^QECdDhHNRsSvVwWX">)> {};
  struct ClassPlainTerm : terminal<cr<R"(^\\\]\[-)">> {};
  struct ClassDashTerm : terminal<lit<"-">> {};
  struct PosixNameTerm : terminal<some(cr<"a-zA-Z_">)> {};
  static constexpr auto PosixTail = some(cr<"a-zA-Z_">) + lit<":]">;
  struct PosixOpenTerm : terminal<lit<"[:"> + &PosixTail> {};
  struct PosixNegOpenTerm : terminal<lit<"[:^"> + &PosixTail> {};
  struct ClassLiteralRule
      : rule<assign<&ast::Literal::text>(
            ref<ClassEscapeTerm> | ref<ClassPlainTerm> | ref<ClassDashTerm> |
            lit<"[">)> {};
  struct PosixCharacterClassRule
      : rule<create<ast::PosixCharacterClass> +
             (flag<&ast::PosixCharacterClass::negate>(ref<PosixNegOpenTerm>) |
              ref<PosixOpenTerm>) +
             assign<&ast::PosixCharacterClass::name>(ref<PosixNameTerm>) +
             lit<":]">> {};
  static constexpr auto ClassRangeTail =
      option(nest<ast::CharacterClassRange, &ast::CharacterClassRange::from>(
          lit<"-"> +
          assign<&ast::CharacterClassRange::to>(ref<ClassLiteralRule>)));
  struct ClassAtomRule : rule<(ref<ClassLiteralRule> + ClassRangeTail) |
                              ref<ClassType> | ref<PosixCharacterClassRule>> {};
  struct BracketLiteral : rule<assign<&ast::Literal::text>(lit<"]">)> {};
  struct BracketAtom : rule<ref<BracketLiteral> + ClassRangeTail> {};
  struct QuotedCharTerm : terminal<any> {};
  struct QuotedChar : rule<assign<&ast::Literal::text>(ref<QuotedCharTerm>)> {};
  static constexpr auto QuoteStop = lit<"\\E"> | eof;
  struct QuotedClassEnd : rule<ref<QuotedChar> + QuoteStop + ClassRangeTail> {};
  static constexpr auto ClassQuoted =
      lit<"\\Q"> + ((many(append<&ast::CharacterClass::atoms>(ref<QuotedChar> +
                                                              !QuoteStop)) +
                     append<&ast::CharacterClass::atoms>(ref<QuotedClassEnd>)) |
                    QuoteStop);
  struct CharacterClassRule
      : rule<create<ast::CharacterClass> + lit<"["> +
             option(flag<&ast::CharacterClass::negate>(lit<"^">)) +
             option(append<&ast::CharacterClass::atoms>(ref<BracketAtom>)) +
             many(ClassQuoted | lit<"\\E"> |
                  append<&ast::CharacterClass::atoms>(ref<ClassAtomRule>)) +
             lit<"]">> {};

  struct PossessiveTag
      : rule<lit<"+">, value<ast::QuantifierMode::possessive>> {};
  struct LazyTag : rule<lit<"?">, value<ast::QuantifierMode::lazy>> {};
  static constexpr auto Annotations =
      many(append<&ast::Quantified::annotations>(Annotation));
  static constexpr auto QuantifierSuffix =
      option(Annotations +
             assign<&ast::Quantified::mode>(ref<PossessiveTag> | ref<LazyTag>));
  struct OptionalTag : rule<lit<"?">, value<ast::QuantifierKind::optional>> {};
  struct StarTag : rule<lit<"*">, value<ast::QuantifierKind::star>> {};
  struct PlusTag : rule<lit<"+">, value<ast::QuantifierKind::plus>> {};
  struct RangeTag : rule<lit<"{">, value<ast::QuantifierKind::range>> {};
  static constexpr auto QuantifierBody =
      Annotations +
      ((assign<&ast::Quantified::kind>(ref<OptionalTag> | ref<StarTag> |
                                       ref<PlusTag>) +
        QuantifierSuffix) |
       (assign<&ast::Quantified::kind>(ref<RangeTag>) + Space +
        ((assign<&ast::Quantified::from>(ref<DigitsTerm>) + Space +
          option(flag<&ast::Quantified::hasComma>(lit<",">) + Space +
                 option(assign<&ast::Quantified::to>(ref<DigitsTerm>)) +
                 Space)) |
         (flag<&ast::Quantified::hasComma>(lit<",">) + Space +
          assign<&ast::Quantified::to>(ref<DigitsTerm>) + Space)) +
        lit<"}"> + QuantifierSuffix));
  static constexpr auto Quantifier =
      option(nest<ast::Quantified, &ast::Quantified::operand>(QuantifierBody));
  struct ElementRule : rule<ref<AtomRule> + Quantifier> {};
  struct QuotedEnd : rule<ref<QuotedChar> + QuoteStop + Quantifier> {};
  static constexpr auto Quoted =
      lit<"\\Q"> +
      ((many(append<&ast::Sequence::items>(ref<QuotedChar> + !QuoteStop)) +
        append<&ast::Sequence::items>(ref<QuotedEnd>)) |
       QuoteStop);
  struct Sequence : rule<create<ast::Sequence> +
                         many(Quoted | append<&ast::Sequence::items>(
                                           ref<ElementRule> | Annotation))> {};
  struct Alternation : rule<create<ast::Alternation> +
                            append<&ast::Alternation::branches>(ref<Sequence>) +
                            many(lit<"|"> + append<&ast::Alternation::branches>(
                                                ref<Sequence>))> {};

  struct LookbehindTag : rule<lit<"<=">, value<ast::GroupKind::lookbehind>> {};
  struct NegLookbehindTag
      : rule<lit<"<!">, value<ast::GroupKind::neg_lookbehind>> {};
  struct NonAtomicBehindTag
      : rule<lit<"<*">, value<ast::GroupKind::non_atomic_lookbehind>> {};
  struct AngleTag : rule<lit<"<">, value<ast::GroupKind::named_angle>> {};
  struct QuoteTag : rule<lit<"'">, value<ast::GroupKind::named_quote>> {};
  struct PythonTag : rule<lit<"P<">, value<ast::GroupKind::named_python>> {};
  struct AheadTag : rule<lit<"=">, value<ast::GroupKind::lookahead>> {};
  struct NegativeTag : rule<lit<"!">, value<ast::GroupKind::neg_lookahead>> {};
  struct NonAtomicAheadTag
      : rule<lit<"*">, value<ast::GroupKind::non_atomic_lookahead>> {};
  struct AtomicTag : rule<lit<">">, value<ast::GroupKind::atomic>> {};
  struct AltTag : rule<lit<"|">, value<ast::GroupKind::alt_group>> {};
  struct NoCaptureTag : rule<lit<":">, value<ast::GroupKind::non_capturing>> {};
  struct LongAhead : rule<lit<"pla:"> | lit<"positive_lookahead:">,
                          value<ast::GroupKind::lookahead>> {};
  struct LongBehind : rule<lit<"plb:"> | lit<"positive_lookbehind:">,
                           value<ast::GroupKind::lookbehind>> {};
  struct LongNegativeAhead : rule<lit<"nla:"> | lit<"negative_lookahead:">,
                                  value<ast::GroupKind::neg_lookahead>> {};
  struct LongNegativeBehind : rule<lit<"nlb:"> | lit<"negative_lookbehind:">,
                                   value<ast::GroupKind::neg_lookbehind>> {};
  struct LongNonAtomicAhead
      : rule<lit<"napla:"> | lit<"non_atomic_positive_lookahead:">,
             value<ast::GroupKind::non_atomic_lookahead>> {};
  struct LongNonAtomicBehind
      : rule<lit<"naplb:"> | lit<"non_atomic_positive_lookbehind:">,
             value<ast::GroupKind::non_atomic_lookbehind>> {};
  struct LongAtomic : rule<lit<"atomic:">, value<ast::GroupKind::atomic>> {};
  struct GroupRule
      : rule<create<ast::Group> +
             ((lit<"(?"> +
               (assign<&ast::Group::kind>(ref<LookbehindTag> |
                                          ref<NegLookbehindTag> |
                                          ref<NonAtomicBehindTag>) |
                (assign<&ast::Group::kind>(ref<AngleTag>) +
                 assign<&ast::Group::name>(ref<NameTerm>) + lit<">">) |
                (assign<&ast::Group::kind>(ref<QuoteTag>) +
                 assign<&ast::Group::name>(ref<NameTerm>) + lit<"'">) |
                (assign<&ast::Group::kind>(ref<PythonTag>) +
                 assign<&ast::Group::name>(ref<NameTerm>) + lit<">">) |
                assign<&ast::Group::kind>(ref<AheadTag> | ref<NegativeTag> |
                                          ref<NonAtomicAheadTag> |
                                          ref<AtomicTag> | ref<AltTag>) |
                (option(flag<&ast::Group::resetFlags>(lit<"^">)) +
                 option(assign<&ast::Group::onFlags>(ref<FlagTerm>)) +
                 option(lit<"-"> +
                        option(assign<&ast::Group::offFlags>(ref<FlagTerm>))) +
                 assign<&ast::Group::kind>(ref<NoCaptureTag>)))) |
              (lit<"(*"> +
               assign<&ast::Group::kind>(
                   ref<LongAhead> | ref<LongBehind> | ref<LongNegativeAhead> |
                   ref<LongNegativeBehind> | ref<LongNonAtomicAhead> |
                   ref<LongNonAtomicBehind> | ref<LongAtomic>)) |
              lit<"(">)+assign<&ast::Group::body>(ref<Alternation>) +
             lit<")">> {};

  struct Backreference
      : rule<assign<&ast::Backreference::number>(ref<BareBackrefTerm>) |
             (lit<"\\g"> +
              ((option(assign<&ast::Backreference::sign>(lit<"-"> | lit<"+">)) +
                assign<&ast::Backreference::number>(ref<DigitsTerm>)) |
               (lit<"{"> + Space +
                option(assign<&ast::Backreference::sign>(lit<"-"> | lit<"+">)) +
                assign<&ast::Backreference::number>(ref<DigitsTerm>) + Space +
                lit<"}">) |
               (lit<"{"> + assign<&ast::Backreference::name>(ref<NameTerm>) +
                lit<"}">))) |
             (lit<"\\k"> +
              ((lit<"<"> + assign<&ast::Backreference::name>(ref<NameTerm>) +
                lit<">">) |
               (lit<"'"> + assign<&ast::Backreference::name>(ref<NameTerm>) +
                lit<"'">) |
               (lit<"{"> + assign<&ast::Backreference::name>(ref<NameTerm>) +
                lit<"}">))) |
             (lit<"(?P="> + assign<&ast::Backreference::name>(ref<NameTerm>) +
              lit<")">)> {};
  static constexpr auto SignedRef =
      option(assign<&ast::SubroutineReference::sign>(lit<"+"> | lit<"-">)) +
      assign<&ast::SubroutineReference::number>(ref<DigitsTerm>);
  struct SubroutineReference
      : rule<(lit<"(?"> +
              (flag<&ast::SubroutineReference::wholePattern>(lit<"R">) |
               SignedRef |
               (lit<"&"> +
                assign<&ast::SubroutineReference::name>(ref<NameTerm>)) |
               (lit<"P>"> +
                assign<&ast::SubroutineReference::name>(ref<NameTerm>))) +
              lit<")">) |
             (lit<"\\g"> +
              ((lit<"<"> +
                assign<&ast::SubroutineReference::name>(ref<NameTerm>) +
                lit<">">) |
               (lit<"'"> +
                assign<&ast::SubroutineReference::name>(ref<NameTerm>) +
                lit<"'">) |
               (lit<"<"> + SignedRef + lit<">">) |
               (lit<"'"> + SignedRef + lit<"'">)))> {};
  struct CalloutRule;
  struct ConditionTerm
      : terminal<(option(cr<"+-">) + some(Digit)) |
                 (lit<"<"> + Name + lit<">">) | (lit<"'"> + Name + lit<"'">) |
                 (lit<"R"> + option(some(Digit))) | (lit<"R&"> + Name) | Name> {
  };
  struct ConditionalPattern
      : rule<create<ast::ConditionalPattern> + lit<"(?"> +
             ((lit<"("> +
               assign<&ast::ConditionalPattern::conditionText>(
                   ref<ConditionTerm>) +
               lit<")">) |
              assign<&ast::ConditionalPattern::conditionNode>(ref<CalloutRule> |
                                                              ref<GroupRule>)) +
             assign<&ast::ConditionalPattern::thenBranch>(ref<Sequence>) +
             option(lit<"|"> + assign<&ast::ConditionalPattern::elseBranch>(
                                   ref<Sequence>)) +
             lit<")">> {};

  static constexpr auto StarOptions =
      (lit<"UTF"> + option(lit<"8"> | lit<"16"> | lit<"32">)) | lit<"UCP"> |
      lit<"NO_AUTO_POSSESS"> | lit<"NO_START_OPT"> | lit<"NO_DOTSTAR_ANCHOR"> |
      lit<"NO_JIT"> | lit<"CRLF"> | lit<"CR"> | lit<"LF"> | lit<"NUL"> |
      lit<"ANYCRLF"> | lit<"ANY"> | (lit<"LIMIT_MATCH="> + some(Digit)) |
      (lit<"LIMIT_RECURSION="> + some(Digit)) |
      (lit<"LIMIT_DEPTH="> + some(Digit)) | (lit<"LIMIT_HEAP="> + some(Digit)) |
      lit<"BSR_ANYCRLF"> | lit<"BSR_UNICODE"> | lit<"NOTEMPTY"> |
      lit<"NOTEMPTY_ATSTART">;
  struct StarOptionTerm : terminal<StarOptions> {};
  struct FlagOptionRule
      : rule<(lit<"^"> + option(ref<FlagTerm>)) |
                 (ref<FlagTerm> + option(lit<"-"> + option(ref<FlagTerm>))) |
                 (lit<"-"> + option(ref<FlagTerm>)),
             std::string_view> {};
  struct OptionSettingRule
      : rule<create<ast::OptionSetting> +
             ((lit<"(*"> +
               assign<&ast::OptionSetting::text>(ref<StarOptionTerm>)) |
              (lit<"(?"> +
               assign<&ast::OptionSetting::text>(ref<FlagOptionRule>))) +
             lit<")">> {};
  struct VerbTerm
      : terminal<lit<"ACCEPT"> | lit<"F"> | lit<"FAIL"> | lit<"MARK"> |
                 lit<"COMMIT"> | lit<"PRUNE"> | lit<"SKIP"> | lit<"THEN">> {};
  struct VerbTextTerm : terminal<some(cr<"^)">)> {};
  struct BacktrackingControlRule
      : rule<create<ast::BacktrackingControl> + lit<"(*"> +
             option(assign<&ast::BacktrackingControl::verb>(ref<VerbTerm>)) +
             option(flag<&ast::BacktrackingControl::hasName>(lit<":">) +
                    option(assign<&ast::BacktrackingControl::name>(
                        ref<VerbTextTerm>))) +
             lit<")">> {};

  template <auto Open, auto Close>
  struct CalloutText
      : terminal<Open + many((Close + Close) | (!Close + any)) + Close,
                 text_conv<callout_text>> {};
  struct CalloutRule
      : rule<create<ast::Callout> + lit<"(?C"> +
             option(assign<&ast::Callout::number>(ref<DigitsTerm>) |
                    flag<&ast::Callout::stringArgument>(
                        assign<&ast::Callout::text>(
                            ref<CalloutText<lit<"'">, lit<"'">>> |
                            ref<CalloutText<lit<"\"">, lit<"\"">>> |
                            ref<CalloutText<lit<"`">, lit<"`">>> |
                            ref<CalloutText<lit<"^">, lit<"^">>> |
                            ref<CalloutText<lit<"%">, lit<"%">>> |
                            ref<CalloutText<lit<"#">, lit<"#">>> |
                            ref<CalloutText<lit<"$">, lit<"$">>> |
                            ref<CalloutText<lit<"{">, lit<"}">>>))) +
             lit<")">> {};

  struct AtomRule
      : rule<ref<OptionSettingRule> | ref<BacktrackingControlRule> |
             ref<CalloutRule> | ref<GroupRule> | ref<Backreference> |
             ref<AmbiguousNumberRule> | ref<SubroutineReference> |
             ref<ConditionalPattern> | ref<CharacterType> |
             ref<CharacterClassRule> | ref<Anchor> | ref<MatchPointReset> | ref<LiteralAtom>> {};
  struct PcreRule : rule<create<ast::Pcre> +
                         assign<&ast::Pcre::pattern>(ref<Alternation>)> {};
  using entry = PcreRule;
};

} // namespace pcre
