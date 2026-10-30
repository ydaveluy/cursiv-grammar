#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "metric/core/ast.hpp"

namespace metric {

struct Metric : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct INTE : terminal<option(cr<"+-">) + some(d)> {};

  // `m` names milli and metre, `T` tera and tesla: one terminal each,
  // referenced from both the prefix and the unit alternation, so the state
  // after it -- not the lexer -- decides which the next token makes it.
  struct METRE_OR_MILLI : terminal<lit<"m">> {};
  struct TESLA_OR_TERA : terminal<lit<"T">> {};

  // Bare: the matched spelling is an enumerator name of whichever member it
  // is assigned to, so no converter is declared here.
  struct PrefixLetter
      : terminal<as<ast::Prefix>(
            lit<"Q"> | lit<"R"> | lit<"Y"> | lit<"Z"> | lit<"E"> | lit<"P"> |
            lit<"G"> | lit<"M"> | lit<"k"> | lit<"h"> | lit<"da"> | lit<"d"> |
            lit<"c"> | lit<"n"> | lit<"p"> | lit<"f"> | lit<"a"> | lit<"z"> |
            lit<"y"> | lit<"r"> | lit<"q">)> {};
  // Not a legal identifier, so its enumerator is named and matched by hand.
  struct PREFIX_MICRO
      : terminal<as<ast::Prefix::micro>(lit<"µ"> | lit<"μ"> | lit<"u">)> {};

  struct UnitWord
      : terminal<as<ast::Unit>(
            lit<"g"> | lit<"s"> | lit<"A"> | lit<"K"> | lit<"mol"> | lit<"cd"> |
            lit<"rad"> | lit<"Hz"> | lit<"sr"> | lit<"N"> | lit<"Pa"> |
            lit<"J"> | lit<"W"> | lit<"C"> | lit<"V"> | lit<"F"> | lit<"S"> |
            lit<"Wb"> | lit<"H"> | lit<"lm"> | lit<"lx"> | lit<"Bq"> |
            lit<"Gy"> | lit<"Sv"> | lit<"kat">)> {};
  struct UNIT_DEGC : terminal<as<ast::Unit::degC>(lit<"°C"> | lit<"˚C">)> {};

  static constexpr auto PrefixTerm = ref<PrefixLetter> | ref<PREFIX_MICRO> |
                                     ref<METRE_OR_MILLI> | ref<TESLA_OR_TERA>;
  static constexpr auto UnitTerm = ref<UnitWord> | as<ast::Unit::ohm>(lit<"Ω"> | lit<"Ω">) |
                                   ref<UNIT_DEGC> | ref<METRE_OR_MILLI> |
                                   ref<TESLA_OR_TERA> | as<ast::Unit::dimensionless>(kw<"1">);

  struct Measure : rule<option(assign<&ast::Measure::prefix>(PrefixTerm) + gap::none) +
                        assign<&ast::Measure::unit>(UnitTerm)> {};

  struct Uom;
  struct Atom : rule<ref<Measure> | (lit<"("> + ref<Uom> + lit<")">)> {};
  struct Power
      : rule<ref<Atom> +
             option(nest<ast::PowerUom, &ast::PowerUom::base>(
                 (lit<"^"> |
                  lit<"**">)+assign<&ast::PowerUom::exponent>(ref<INTE>)))> {};
  struct Uom : infix<Power, ast::BinaryUom,
                     left(as<ast::Operator::multiply>(lit<"*"> | lit<"·">) |
                          as<ast::Operator::divide>(lit<"/">))> {};

  using entry = Uom;
  using ignored = WS;
};

} // namespace metric
