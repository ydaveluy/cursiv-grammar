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
  struct AGE : terminal<some(d)> {};
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
  struct UNIX_VARIANTS
      : terminal<as<unix_variants>(lit<"U"> + many(cr<"ABCHILOSUVX*">))> {};

  struct NOTRIGID : terminal<as<ast::Qualifier::not_rigid>(lit<"@">)> {};
  struct NOKNOWLEDGE : terminal<as<ast::Qualifier::no_knowledge>(lit<"?">)> {};

  struct Crossover : rule<create<ast::Crossover>(
      lit<"("> + option(gap::none + assign<&ast::Crossover::run>(ref<SIZING>)) +
      gap::none + lit<")">)> {};
  static constexpr auto Modifier = ref<SIZING> | ref<Crossover> |
      ref<NOTRIGID> | as<ast::Qualifier::wannabe>(lit<">">) | as<ast::Qualifier::for_living>(lit<"$">) | ref<NOKNOWLEDGE> | as<ast::Qualifier::refused>(lit<"!">);

  struct Unix : rule<assign<&ast::Unix::variants>(ref<UNIX_VARIANTS>)> {};
  static constexpr auto ShapeModifiers =
      many(gap::none + append<&ast::ShapeSegment::modifiers>(Modifier));
  struct FirstShapeSegment : rule<create<ast::ShapeSegment>(lit<"s"> + ShapeModifiers)> {};
  struct NextShapeSegment : rule<create<ast::ShapeSegment>(lit<":"> + ShapeModifiers)> {};
  struct Shape : rule<create<ast::Shape>(
      append<&ast::Shape::segments>(ref<FirstShapeSegment>) +
      many(gap::none + append<&ast::Shape::segments>(ref<NextShapeSegment>)))> {};
  struct Tuple : rule<create<ast::Tuple>(
      option(flag<&ast::Tuple::refused>(lit<"!">) + gap::none) +
      ((assign<&ast::Tuple::kind>(lit<"a">) + gap::none +
        assign<&ast::Tuple::exactAge>(ref<AGE>)) |
       assign<&ast::Tuple::kind>(ref<CATEGORY> | ref<Shape> | ref<Unix>)) +
      many(gap::none + append<&ast::Tuple::modifiers>(Modifier)))> {};
  struct File : rule<create<ast::File>(
      option(lit<"-----BEGIN GEEK CODE BLOCK-----"> +
             option(lit<"Version:"> + assign<&ast::File::version>(ref<VERSION>))) +
      assign<&ast::File::header>(ref<GCODE>) +
      many(append<&ast::File::tuples>(ref<Tuple>)) +
      option(lit<"------END GEEK CODE BLOCK------">))> {};

  using entry = File;
  using ignored = WS;
};

}  // namespace geekcode
