#pragma once

#include <charconv>
#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pcre/core/ast.hpp"

namespace pcre {

inline constexpr auto comment_text = [](std::string_view text) {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (text.size() < 4 || !text.starts_with("(?#") || text.back() != ')')
    return Result::err("Incomplete pattern comment.");
  const auto body = text.substr(3, text.size() - 4);
  if (body.find(')') != body.npos)
    return Result::err("A pattern comment ends at its first closing parenthesis.");
  return Result::ok(body);
};
inline constexpr auto callout_text =
    [](std::string_view text, std::pmr::memory_resource *memory) {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (text.size() < 2)
    return Result::err("Incomplete callout string.");
  const char open = text.front();
  const char close = open == '{' ? '}' : open;
  if (std::string_view("'\"`^%#${").find(open) == std::string_view::npos || text.back() != close)
    return Result::err("Invalid callout string delimiters.");
  const auto body = text.substr(1, text.size() - 2);
  for (std::size_t i = 0; i < body.size(); ++i)
    if (body[i] == close && (++i == body.size() || body[i] != close))
      return Result::err("A callout string delimiter must be doubled inside its text.");
  if (body.empty())
    return Result::ok(body);
  const auto room = cursiv::text_room(memory, body.size());
  std::size_t size = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[size++] = body[i];
    if (body[i] == close)
      ++i;
  }
  return Result::ok({room.data(), size});
};
inline constexpr auto backref_number = [](std::string_view text) {
  using Result = cursiv::ConversionResult<std::int64_t>;
  if (text.size() < 2 || text.front() != '\\' || text[1] < '1' || text[1] > '9')
    return Result::err("A backreference needs a backslash and a positive decimal number.");
  std::int64_t value = 0;
  const auto digits = text.substr(1);
  const auto parsed = std::from_chars(digits.data(), digits.data() + digits.size(), value);
  if (parsed.ec != std::errc{} || parsed.ptr != digits.data() + digits.size())
    return Result::err("Invalid or out-of-range decimal backreference.");
  return Result::ok(value);
};

inline constexpr auto class_set_operator = [](std::string_view text) {
  return text.substr(0, 1);
};

struct Pcre : cursiv::GrammarSpec {
  struct Alternation;
  struct Atom;
  struct Group;
  struct CharacterClass;

  static constexpr auto Digit = cr<"0-9">;
  static constexpr auto Hex = cr<"0-9a-fA-F">;
  static constexpr auto Space = many(cr<" \t">);
  static constexpr auto Name = some(uc::L | uc::Nd | lit<"_">);
  struct NameTerm : terminal<Name> {};
  struct DigitsTerm : terminal<as<std::int64_t>(some(Digit))> {};
  struct FlagTerm : terminal<some(cr<"aimnrsxJUDPTSW">)> {};

  struct CommentTerm
      : terminal<as<comment_text>(lit<"(?#"> + many(cr<"^)">) + lit<")">)> {};
  struct Comment : rule<assign<&ast::Comment::text>(ref<CommentTerm>)> {};
  struct QuoteBoundaryTerm : terminal<lit<"\\Q\\E"> | lit<"\\E">> {};
  struct QuoteBoundary
      : rule<assign<&ast::QuoteBoundary::text>(ref<QuoteBoundaryTerm>)> {};
  static constexpr auto Annotation = ref<Comment> | ref<QuoteBoundary>;

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
  struct AmbiguousNumber
      : rule<assign<&ast::NumericEscape::text>(ref<AmbiguousNumberTerm>)> {};
  struct BareBackrefTerm
      : terminal<as<backref_number>(
            lit<"\\"> + ((cr<"1-7"> + !Digit) | (cr<"89"> + many(Digit))))> {};

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
  struct ClassLiteral
      : rule<assign<&ast::Literal::text>(
            ref<ClassEscapeTerm> | ref<ClassPlainTerm> | ref<ClassDashTerm> |
            lit<"[">)> {};
  struct PosixCharacterClass
      : rule<create<ast::PosixCharacterClass> +
             (flag<&ast::PosixCharacterClass::negate>(ref<PosixNegOpenTerm>) |
              ref<PosixOpenTerm>) +
             assign<&ast::PosixCharacterClass::name>(ref<PosixNameTerm>) +
             lit<":]">> {};
  static constexpr auto ClassRangeTail =
      option(nest<ast::CharacterClassRange, &ast::CharacterClassRange::from>(
          lit<"-"> +
          assign<&ast::CharacterClassRange::to>(ref<ClassLiteral>)));
  struct ClassAtom : rule<(ref<ClassLiteral> + ClassRangeTail) |
                              ref<ClassType> | ref<PosixCharacterClass>> {};
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
  struct CharacterClass
      : rule<create<ast::CharacterClass> + lit<"["> +
             option(flag<&ast::CharacterClass::negate>(lit<"^">)) +
             option(append<&ast::CharacterClass::atoms>(ref<BracketAtom>)) +
             many(ClassQuoted | lit<"\\E"> |
                  append<&ast::CharacterClass::atoms>(ref<ClassAtom>)) +
             lit<"]">> {};

  // Perl (?[...]) has intrinsic /xx inside this one class. A nested ordinary
  // class still quotes characters with \Q...\E, including spaces and tabs.
  struct ExtendedSpaceTerm : terminal<some(cr<" \t">)> {};
  static constexpr auto ExtendedPadding =
      many(ref<ExtendedSpaceTerm> | lit<"\\E"> | lit<"\\Q\\E">);
  struct ExtendedClassPlainTerm : terminal<cr<R"(^\\\]\[ \t-)">> {};
  struct ExtendedClassLiteral
      : rule<assign<&ast::Literal::text>(
            ref<ClassEscapeTerm> | ref<ExtendedClassPlainTerm> |
            ref<ClassDashTerm> | lit<"[">)> {};
  static constexpr auto ExtendedClassRangeTail =
      option(nest<ast::CharacterClassRange, &ast::CharacterClassRange::from>(
          ExtendedPadding + lit<"-"> + ExtendedPadding +
          assign<&ast::CharacterClassRange::to>(ref<ExtendedClassLiteral>)));
  struct ExtendedClassAtom
      : rule<(ref<ExtendedClassLiteral> + ExtendedClassRangeTail) |
             ref<ClassType> | ref<PosixCharacterClass>> {};
  struct ExtendedBracketAtom
      : rule<ref<BracketLiteral> + ExtendedClassRangeTail> {};
  struct ExtendedQuotedClassEnd
      : rule<ref<QuotedChar> + QuoteStop + ExtendedClassRangeTail> {};
  static constexpr auto ExtendedClassQuoted =
      lit<"\\Q"> +
      ((many(append<&ast::CharacterClass::atoms>(ref<QuotedChar> + !QuoteStop)) +
        append<&ast::CharacterClass::atoms>(ref<ExtendedQuotedClassEnd>)) |
       QuoteStop);
  struct ExtendedOrdinaryClass
      : rule<create<ast::CharacterClass> + lit<"["> +
             ExtendedPadding +
             option(flag<&ast::CharacterClass::negate>(lit<"^">) +
                    ExtendedPadding) +
             option(append<&ast::CharacterClass::atoms>(
                        ref<ExtendedBracketAtom>) + ExtendedPadding) +
             many((ExtendedClassQuoted |
                   append<&ast::CharacterClass::atoms>(ref<ExtendedClassAtom>)) +
                  ExtendedPadding) + lit<"]">> {};

  struct ClassSetExpression;
  struct ExtendedEscape
      : rule<assign<&ast::Literal::text>(ref<ClassEscapeTerm>)> {};
  struct ClassSetPrimary
      : rule<(ref<ExtendedEscape> | ref<ClassType> | ref<PosixCharacterClass> |
             ref<ExtendedOrdinaryClass> |
             create<ast::ClassSetGroup>(
                 lit<"("> + ExtendedPadding +
                 assign<&ast::ClassSetGroup::expression>(
                     ref<ClassSetExpression>) + lit<")">)) + ExtendedPadding> {};
  struct ClassSetExpression
      : infix<ClassSetPrimary, ast::ClassSetBinary,
              levels(prefix<ast::ClassSetUnary>(
                         as<class_set_operator>(lit<"!"> + ExtendedPadding)),
                     left(as<class_set_operator>(lit<"&"> + ExtendedPadding)),
                     left(as<class_set_operator>(
                         (lit<"+"> | lit<"|"> | lit<"-"> | lit<"^">) +
                         ExtendedPadding)))> {};
  struct ExtendedCharacterClass
      : rule<create<ast::ExtendedCharacterClass> + lit<"(?["> +
             ExtendedPadding +
             assign<&ast::ExtendedCharacterClass::expression>(
                 ref<ClassSetExpression>) + lit<"])">> {};

  static constexpr auto Annotations =
      many(append<&ast::Quantified::annotations>(Annotation));
  static constexpr auto QuantifierSuffix =
      option(Annotations +
             assign<&ast::Quantified::mode>(as<ast::QuantifierMode::possessive>(lit<"+">) | as<ast::QuantifierMode::lazy>(lit<"?">)));
  static constexpr auto QuantifierBody =
      Annotations +
      ((assign<&ast::Quantified::kind>(as<ast::QuantifierKind::optional>(lit<"?">) | as<ast::QuantifierKind::star>(lit<"*">) |
                                       as<ast::QuantifierKind::plus>(lit<"+">)) +
        QuantifierSuffix) |
       (assign<&ast::Quantified::kind>(as<ast::QuantifierKind::range>(lit<"{">)) + Space +
        ((assign<&ast::Quantified::from>(ref<DigitsTerm>) + Space +
          option(flag<&ast::Quantified::hasComma>(lit<",">) + Space +
                 option(assign<&ast::Quantified::to>(ref<DigitsTerm>)) +
                 Space)) |
         (flag<&ast::Quantified::hasComma>(lit<",">) + Space +
          assign<&ast::Quantified::to>(ref<DigitsTerm>) + Space)) +
        lit<"}"> + QuantifierSuffix));
  static constexpr auto Quantifier =
      option(nest<ast::Quantified, &ast::Quantified::operand>(QuantifierBody));
  struct Element : rule<ref<Atom> + Quantifier> {};
  struct QuotedEnd : rule<ref<QuotedChar> + QuoteStop + Quantifier> {};
  static constexpr auto Quoted =
      lit<"\\Q"> +
      ((many(append<&ast::Sequence::items>(ref<QuotedChar> + !QuoteStop)) +
        append<&ast::Sequence::items>(ref<QuotedEnd>)) |
       QuoteStop);
  struct Sequence : rule<create<ast::Sequence> +
                         many(Quoted | append<&ast::Sequence::items>(
                                           ref<Element> | Annotation))> {};
  struct Alternation : rule<create<ast::Alternation> +
                            append<&ast::Alternation::branches>(ref<Sequence>) +
                            many(lit<"|"> + append<&ast::Alternation::branches>(
                                                ref<Sequence>))> {};

  struct Group
      : rule<create<ast::Group> +
             ((lit<"(?"> +
               (assign<&ast::Group::kind>(as<ast::GroupKind::lookbehind>(lit<"<=">) |
                                          as<ast::GroupKind::neg_lookbehind>(lit<"<!">) |
                                          as<ast::GroupKind::non_atomic_lookbehind>(lit<"<*">)) |
                (assign<&ast::Group::kind>(as<ast::GroupKind::named_angle>(lit<"<">)) +
                 assign<&ast::Group::name>(ref<NameTerm>) + lit<">">) |
                (assign<&ast::Group::kind>(as<ast::GroupKind::named_quote>(lit<"'">)) +
                 assign<&ast::Group::name>(ref<NameTerm>) + lit<"'">) |
                (assign<&ast::Group::kind>(as<ast::GroupKind::named_python>(lit<"P<">)) +
                 assign<&ast::Group::name>(ref<NameTerm>) + lit<">">) |
                assign<&ast::Group::kind>(as<ast::GroupKind::lookahead>(lit<"=">) | as<ast::GroupKind::neg_lookahead>(lit<"!">) |
                                          as<ast::GroupKind::non_atomic_lookahead>(lit<"*">) |
                                          as<ast::GroupKind::atomic>(lit<">">) | as<ast::GroupKind::alt_group>(lit<"|">)) |
                (option(flag<&ast::Group::resetFlags>(lit<"^">)) +
                 option(assign<&ast::Group::onFlags>(ref<FlagTerm>)) +
                 option(lit<"-"> +
                        option(assign<&ast::Group::offFlags>(ref<FlagTerm>))) +
                 assign<&ast::Group::kind>(as<ast::GroupKind::non_capturing>(lit<":">))))) |
              (lit<"(*"> +
               assign<&ast::Group::kind>(
                   as<ast::GroupKind::lookahead>(lit<"pla:"> | lit<"positive_lookahead:">) | as<ast::GroupKind::lookbehind>(lit<"plb:"> | lit<"positive_lookbehind:">) | as<ast::GroupKind::neg_lookahead>(lit<"nla:"> | lit<"negative_lookahead:">) |
                   as<ast::GroupKind::neg_lookbehind>(lit<"nlb:"> | lit<"negative_lookbehind:">) | as<ast::GroupKind::non_atomic_lookahead>(lit<"napla:"> | lit<"non_atomic_positive_lookahead:">) |
                   as<ast::GroupKind::non_atomic_lookbehind>(lit<"naplb:"> | lit<"non_atomic_positive_lookbehind:">) | as<ast::GroupKind::atomic>(lit<"atomic:">))) |
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
               (lit<"{"> + Space + assign<&ast::Backreference::name>(ref<NameTerm>) +
                Space + lit<"}">))) |
             (lit<"\\k"> +
              ((lit<"<"> + assign<&ast::Backreference::name>(ref<NameTerm>) +
                lit<">">) |
               (lit<"'"> + assign<&ast::Backreference::name>(ref<NameTerm>) +
                lit<"'">) |
               (lit<"{"> + Space + assign<&ast::Backreference::name>(ref<NameTerm>) +
                Space + lit<"}">))) |
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
                (SignedRef | assign<&ast::SubroutineReference::name>(ref<NameTerm>)) +
                lit<">">) |
               (lit<"'"> +
                (SignedRef | assign<&ast::SubroutineReference::name>(ref<NameTerm>)) +
                lit<"'">)))> {};
  struct Callout;
  struct ConditionTerm
      : terminal<(option(cr<"+-">) + some(Digit)) |
                 (lit<"<"> + Name + lit<">">) | (lit<"'"> + Name + lit<"'">) |
                 (lit<"R"> + many(Digit)) | (lit<"R&"> + Name) | Name> {
  };
  struct ConditionalPattern
      : rule<create<ast::ConditionalPattern> + lit<"(?"> +
             ((lit<"("> +
               assign<&ast::ConditionalPattern::conditionText>(
                   ref<ConditionTerm>) +
               lit<")">) |
              assign<&ast::ConditionalPattern::conditionNode>(ref<Callout> |
                                                              ref<Group>)) +
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
  struct FlagOption
      : rule<as<std::string_view>(
            (lit<"^"> + option(ref<FlagTerm>)) |
            (ref<FlagTerm> + option(lit<"-"> + option(ref<FlagTerm>))) |
            (lit<"-"> + option(ref<FlagTerm>)))> {};
  struct OptionSetting
      : rule<create<ast::OptionSetting> +
             ((lit<"(*"> +
               assign<&ast::OptionSetting::text>(ref<StarOptionTerm>)) |
              (lit<"(?"> +
               assign<&ast::OptionSetting::text>(ref<FlagOption>))) +
             lit<")">> {};
  struct VerbTerm
      : terminal<lit<"ACCEPT"> | lit<"F"> | lit<"FAIL"> | lit<"MARK"> |
                 lit<"COMMIT"> | lit<"PRUNE"> | lit<"SKIP"> | lit<"THEN">> {};
  struct VerbTextTerm : terminal<some(cr<"^)">)> {};
  struct BacktrackingControl
      : rule<create<ast::BacktrackingControl> + lit<"(*"> +
             option(assign<&ast::BacktrackingControl::verb>(ref<VerbTerm>)) +
             option(flag<&ast::BacktrackingControl::hasName>(lit<":">) +
                    option(assign<&ast::BacktrackingControl::name>(
                        ref<VerbTextTerm>))) +
             lit<")">> {};

  template <auto Open, auto Close>
  struct CalloutText
      : terminal<as<callout_text>(
            Open + many((Close + Close) | (!Close + any)) + Close)> {};
  struct Callout
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

  struct Atom
      : rule<ref<OptionSetting> | ref<BacktrackingControl> |
             ref<Callout> | ref<Group> | ref<Backreference> |
             ref<AmbiguousNumber> | ref<SubroutineReference> |
             ref<ConditionalPattern> | ref<CharacterType> |
             ref<ExtendedCharacterClass> | ref<CharacterClass> |
             ref<Anchor> | ref<MatchPointReset> | ref<LiteralAtom>> {};
  struct PcreRule : rule<create<ast::Pcre> +
                         assign<&ast::Pcre::pattern>(ref<Alternation>)> {};
  using entry = PcreRule;
};

} // namespace pcre
