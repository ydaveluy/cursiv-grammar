// The jam language: its grammar.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "jam/core/ast.hpp"

namespace jam {

inline constexpr auto accidental_of =
    [](std::string_view t) -> ast::Accidental {
  return t == "b" ? ast::Accidental::flat : ast::Accidental::sharp;
};

inline constexpr auto quality_of = [](std::string_view t) -> ast::Quality {
  if (t == "maj") return ast::Quality::maj;
  if (t == "min") return ast::Quality::min;
  if (t == "dim") return ast::Quality::dim;
  if (t == "aug") return ast::Quality::aug;
  if (t == "m") return ast::Quality::m;
  if (t == "M") return ast::Quality::M;
  if (t == "P") return ast::Quality::P;
  if (t == "sus") return ast::Quality::sus;
  if (t == "+") return ast::Quality::aug_plus;
  return ast::Quality::d;
};

inline constexpr auto sign_of = [](std::string_view t) -> ast::Sign {
  return t == "+" ? ast::Sign::plus : ast::Sign::minus;
};

struct Jam : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
  // Native comments start in column one. Newline trivia can consume one;
  // the initial line is handled explicitly by the entry rule.
  struct LINE_BREAK : terminal<eol + option(ref<COMMENT>)> {};

  struct KEY : terminal<cr<"A-G">> {};
  struct NUM : terminal<as<std::int64_t>(some(d))> {};
  struct ACCIDENTAL : terminal<as<accidental_of>(lit<"b"> | lit<"#">)> {};
  struct QUALITY : terminal<as<quality_of>(lit<"maj"> | lit<"min"> |
                                           lit<"dim"> | lit<"aug"> | lit<"m"> |
                                           lit<"M"> | lit<"P"> | lit<"d"> |
                                           lit<"sus"> | lit<"+", !d>)> {};
  struct SIGN : terminal<as<sign_of>(lit<"+"> | lit<"-">)> {};

  struct Note
      : rule<assign<&ast::Note::key>(ref<KEY>) +
             option(gap::none + assign<&ast::Note::accidental>(ref<ACCIDENTAL>))> {};

  struct Bass : rule<lit<"/"> + gap::none + assign<&ast::Bass::key>(ref<KEY>) +
      option(gap::none + assign<&ast::Bass::accidental>(ref<ACCIDENTAL>))> {};

  // Both optional groups below share their opening token ('-' and '/') with a
  // sibling item: "C - /" needs the '-' read as a Rest once '/' proves it is
  // not an alteration's sign, so this stays a plain option() -- a GLR fork,
  // not a commit -- and lets the dead branch die on that token.
  static constexpr auto chordQuality =
      option(gap::none + assign<&ast::Chord::quality>(ref<QUALITY>)) +
      option(gap::none + assign<&ast::Chord::extension>(ref<NUM>)) +
      option(gap::none + assign<&ast::Chord::alterationSign>(ref<SIGN>) +
             gap::none + assign<&ast::Chord::alteration>(ref<NUM>));

  struct Chord
      : rule<assign<&ast::Chord::note>(ref<Note>) + chordQuality +
             option(gap::none + assign<&ast::Chord::bass>(ref<Bass>))> {};

  struct RepeatChord : rule<create<ast::RepeatChord> + lit<"/">> {};
  struct RestRule : rule<create<ast::Rest> + lit<"-">> {};
  struct ExtendChord : rule<create<ast::ExtendChord> + lit<"=">> {};
  struct PERCENT_RUN : terminal<as<[](std::string_view text) {
      return static_cast<std::int64_t>(text.size());
    }>(some(lit<"%">))> {};
  struct RepeatMeasure : rule<assign<&ast::RepeatMeasure::count>(ref<PERCENT_RUN>)> {};

  struct Item
      : rule<ref<Chord> | ref<RepeatChord> | ref<RestRule> |
             ref<ExtendChord> | ref<RepeatMeasure>> {};

  struct Meter
      : rule<lit<"["> + assign<&ast::Meter::numerator>(ref<NUM>) + lit<"/"> +
             assign<&ast::Meter::denominator>(ref<NUM>) + lit<"]">> {};

  static constexpr auto measureItems =
      some(append<&ast::Measure::items>(ref<Item>));

  struct Measure
      : rule<option(assign<&ast::Measure::meter>(ref<Meter>)) +
             measureItems> {};

  struct MeteredMeasure
      : rule<assign<&ast::Measure::meter>(ref<Meter>) + measureItems> {};

  static constexpr auto barredMeasures =
      many(lit<"|"> + append<&ast::Section::measures>(ref<Measure>));

  struct Section
      : rule<append<&ast::Section::measures>(ref<Measure>) +
             barredMeasures> {};

  struct MeteredSection
      : rule<append<&ast::Section::measures>(ref<MeteredMeasure>) +
             barredMeasures> {};

  // A meter change with no bar before it starts a new section: nothing
  // else forces one, since a bare item just extends the open measure.
  struct JamRule
      : rule<option(gap::none + ref<COMMENT>) +
             option(append<&ast::Jam::sections>(ref<Section>) +
                    many(append<&ast::Jam::sections>(
                        ref<MeteredSection>)))> {};

  using entry = JamRule;
  using ignored = list<WS, LINE_BREAK>;
};

}  // namespace jam
