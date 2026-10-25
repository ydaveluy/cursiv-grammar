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
using Accidental_ = cursiv::text_conv<accidental_of>;

inline constexpr auto quality_of = [](std::string_view t) -> ast::Quality {
  if (t == "maj") return ast::Quality::maj;
  if (t == "min") return ast::Quality::min;
  if (t == "dim") return ast::Quality::dim;
  if (t == "aug") return ast::Quality::aug;
  if (t == "m") return ast::Quality::m;
  if (t == "M") return ast::Quality::M;
  if (t == "P") return ast::Quality::P;
  return ast::Quality::d;
};
using Quality_ = cursiv::text_conv<quality_of>;

inline constexpr auto sign_of = [](std::string_view t) -> ast::Sign {
  return t == "+" ? ast::Sign::plus : ast::Sign::minus;
};
using Sign_ = cursiv::text_conv<sign_of>;

struct Jam : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  // '#' with nothing before the next line end is not a comment: the mandatory
  // some() below fails, and the bare '#' terminal (a sharp) is read instead.
  struct COMMENT : terminal<lit<"#"> + some(cr<R"(^\r\n)">)> {};

  struct KEY : terminal<cr<"A-G">> {};
  struct NUM : terminal<some(d), std::int64_t> {};
  struct ACCIDENTAL : terminal<lit<"b"> | lit<"#">, Accidental_> {};
  struct QUALITY
      : terminal<lit<"maj"> | lit<"min"> | lit<"dim"> | lit<"aug"> |
                     lit<"m"> | lit<"M"> | lit<"P"> | lit<"d">,
                 Quality_> {};
  struct SIGN : terminal<lit<"+"> | lit<"-">, Sign_> {};

  struct NoteRule
      : rule<assign<&ast::Note::key>(ref<KEY>) +
             option(assign<&ast::Note::accidental>(ref<ACCIDENTAL>))> {};

  struct BassRule : rule<lit<"/"> + assign<&ast::Bass::key>(ref<KEY>)> {};

  // Both optional groups below share their opening token ('-' and '/') with a
  // sibling item: "C - /" needs the '-' read as a Rest once '/' proves it is
  // not an alteration's sign, so this stays a plain option() -- a GLR fork,
  // not a commit -- and lets the dead branch die on that token.
  static constexpr auto chordQuality =
      option(assign<&ast::Chord::quality>(ref<QUALITY>)) +
      option(assign<&ast::Chord::extension>(ref<NUM>)) +
      option(assign<&ast::Chord::alterationSign>(ref<SIGN>) +
             assign<&ast::Chord::alteration>(ref<NUM>));

  struct ChordRule
      : rule<assign<&ast::Chord::note>(ref<NoteRule>) + chordQuality +
             option(assign<&ast::Chord::bass>(ref<BassRule>))> {};

  struct RepeatChordRule : rule<create<ast::RepeatChord> + lit<"/">> {};
  struct RestRule : rule<create<ast::Rest> + lit<"-">> {};
  struct ExtendChordRule : rule<create<ast::ExtendChord> + lit<"=">> {};
  struct RepeatMeasureRule : rule<create<ast::RepeatMeasure> + lit<"%">> {};

  struct ItemRule
      : rule<ref<ChordRule> | ref<RepeatChordRule> | ref<RestRule> |
             ref<ExtendChordRule> | ref<RepeatMeasureRule>> {};

  struct MeterRule
      : rule<lit<"["> + assign<&ast::Meter::numerator>(ref<NUM>) + lit<"/"> +
             assign<&ast::Meter::denominator>(ref<NUM>) + lit<"]">> {};

  static constexpr auto measureItems =
      some(append<&ast::Measure::items>(ref<ItemRule>));

  struct MeasureRule
      : rule<option(assign<&ast::Measure::meter>(ref<MeterRule>)) +
             measureItems> {};

  struct MeteredMeasureRule
      : rule<assign<&ast::Measure::meter>(ref<MeterRule>) + measureItems> {};

  static constexpr auto barredMeasures =
      many(lit<"|"> + append<&ast::Section::measures>(ref<MeasureRule>));

  struct SectionRule
      : rule<append<&ast::Section::measures>(ref<MeasureRule>) +
             barredMeasures> {};

  struct MeteredSectionRule
      : rule<append<&ast::Section::measures>(ref<MeteredMeasureRule>) +
             barredMeasures> {};

  // A meter change with no bar before it starts a new section: nothing
  // else forces one, since a bare item just extends the open measure.
  struct JamRule
      : rule<option(append<&ast::Jam::sections>(ref<SectionRule>) +
                    many(append<&ast::Jam::sections>(
                        ref<MeteredSectionRule>)))> {};

  using entry = JamRule;
  using ignore = list<WS, COMMENT>;
};

}  // namespace jam
