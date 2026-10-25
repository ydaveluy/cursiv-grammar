#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "geekcode/core/ast.hpp"

namespace geekcode {

inline constexpr auto unix_variants = [](std::string_view text) {
  return text.substr(1);
};

struct Geekcode : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct GCODE : terminal<lit<"G"> + some(cr<"A-Z!">) +
                          many(lit<"/"> + some(cr<"A-Z!">))> {};
  struct VERSION : terminal<some(cr<"0-9.">)> {};
  struct SIZING : terminal<some(cr<"+*-">) | lit<"%"> | lit<"pu"> | lit<"x">> {};
  struct CATEGORY : terminal<
      lit<"d"> | lit<"a"> | lit<"H"> | lit<"C"> | lit<"P"> | lit<"E"> |
      lit<"W"> | lit<"N"> | lit<"o"> | lit<"K"> | lit<"O"> | lit<"M"> |
      lit<"V"> | lit<"PS"> | lit<"PE"> | lit<"Y"> | lit<"PGP"> | lit<"t"> |
      lit<"5"> | lit<"X"> | lit<"R"> | lit<"tv"> | lit<"b"> | lit<"DI"> |
      lit<"D"> | lit<"e"> | lit<"h"> | lit<"r"> | lit<"y"> | lit<"z"> |
      lit<"x"> | lit<"L"> | lit<"g"> | lit<"p"> | lit<"au"> | lit<"w"> |
      lit<"v"> | lit<"3"> | lit<"j"> | lit<"G"> | lit<"B"> | lit<"u"> |
      lit<"f"> | lit<"n">> {};
  struct UNIX_VARIANTS : terminal<lit<"U"> + many(cr<"ABCHILOSUVX*">),
                                  cursiv::text_conv<unix_variants>> {};

  struct NOTRIGID : terminal<lit<"@">, value<ast::Qualifier::not_rigid>> {};
  struct WANNABE : terminal<lit<">">, value<ast::Qualifier::wannabe>> {};
  struct FORLIVING : terminal<lit<"$">, value<ast::Qualifier::for_living>> {};
  struct NOKNOWLEDGE : terminal<lit<"?">, value<ast::Qualifier::no_knowledge>> {};
  struct REFUSED : terminal<lit<"!">, value<ast::Qualifier::refused>> {};

  struct CrossoverRule : rule<create<ast::Crossover>(
      lit<"("> + option(gap::none + assign<&ast::Crossover::run>(ref<SIZING>)) +
      gap::none + lit<")">)> {};
  static constexpr auto Modifier = ref<SIZING> | ref<CrossoverRule> |
      ref<NOTRIGID> | ref<WANNABE> | ref<FORLIVING> | ref<NOKNOWLEDGE> | ref<REFUSED>;

  struct UnixRule : rule<create<ast::Unix>(
      assign<&ast::Unix::variants>(ref<UNIX_VARIANTS>))> {};
  static constexpr auto ShapeModifiers =
      many(gap::none + append<&ast::ShapeSegment::modifiers>(Modifier));
  struct FirstShapeSegment : rule<create<ast::ShapeSegment>(lit<"s"> + ShapeModifiers)> {};
  struct NextShapeSegment : rule<create<ast::ShapeSegment>(lit<":"> + ShapeModifiers)> {};
  struct ShapeRule : rule<create<ast::Shape>(
      append<&ast::Shape::segments>(ref<FirstShapeSegment>) +
      many(gap::none + append<&ast::Shape::segments>(ref<NextShapeSegment>)))> {};
  struct TupleRule : rule<create<ast::Tuple>(
      option(flag<&ast::Tuple::refused>(lit<"!">) + gap::none) +
      assign<&ast::Tuple::kind>(ref<CATEGORY> | ref<ShapeRule> | ref<UnixRule>) +
      many(gap::none + append<&ast::Tuple::modifiers>(Modifier)))> {};
  struct FileRule : rule<create<ast::File>(
      option(lit<"-----BEGIN GEEK CODE BLOCK-----"> +
             option(lit<"Version:"> + assign<&ast::File::version>(ref<VERSION>))) +
      assign<&ast::File::header>(ref<GCODE>) +
      many(append<&ast::File::tuples>(ref<TupleRule>)) +
      option(lit<"------END GEEK CODE BLOCK------">))> {};

  using entry = FileRule;
  using ignore = WS;
};

}  // namespace geekcode
