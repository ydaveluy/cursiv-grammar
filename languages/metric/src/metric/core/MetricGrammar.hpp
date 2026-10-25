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
      : terminal<lit<"Q"> | lit<"R"> | lit<"Y"> | lit<"Z"> | lit<"E"> |
                     lit<"P"> | lit<"G"> | lit<"M"> | lit<"k"> | lit<"h"> |
                     lit<"da"> | lit<"d"> | lit<"c"> | lit<"n"> | lit<"p"> |
                     lit<"f"> | lit<"a"> | lit<"z"> | lit<"y"> | lit<"r"> |
                     lit<"q">,
                 ast::Prefix> {};
  // Not a legal identifier, so its enumerator is named and matched by hand.
  struct PREFIX_MICRO
      : terminal<lit<"µ"> | lit<"μ"> | lit<"u">, value<ast::Prefix::micro>> {};

  struct UnitWord
      : terminal<lit<"g"> | lit<"s"> | lit<"A"> | lit<"K"> | lit<"mol"> |
                     lit<"cd"> | lit<"rad"> | lit<"Hz"> | lit<"sr"> | lit<"N"> |
                     lit<"Pa"> | lit<"J"> | lit<"W"> | lit<"C"> | lit<"V"> |
                     lit<"F"> | lit<"S"> | lit<"Wb"> | lit<"H"> | lit<"lm"> |
                     lit<"lx"> | lit<"Bq"> | lit<"Gy"> | lit<"Sv"> | lit<"kat">,
                 ast::Unit> {};
  struct UNIT_OHM : terminal<lit<"Ω">, value<ast::Unit::ohm>> {};
  struct UNIT_DEGC : terminal<lit<"°C"> | lit<"˚C">, value<ast::Unit::degC>> {};

  struct UNIT_ONE : terminal<lit<"1">, value<ast::Unit::dimensionless>> {};

  struct MUL : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIV : terminal<lit<"/">, value<ast::Operator::divide>> {};

  static constexpr auto PrefixTerm = ref<PrefixLetter> | ref<PREFIX_MICRO> |
                                     ref<METRE_OR_MILLI> | ref<TESLA_OR_TERA>;
  static constexpr auto UnitTerm = ref<UnitWord> | ref<UNIT_OHM> |
                                   ref<UNIT_DEGC> | ref<METRE_OR_MILLI> |
                                   ref<TESLA_OR_TERA> | ref<UNIT_ONE>;

  struct Measure : rule<option(assign<&ast::Measure::prefix>(PrefixTerm)) +
                        assign<&ast::Measure::unit>(UnitTerm)> {};

  struct Uom;
  struct Atom : rule<ref<Measure> | (lit<"("> + ref<Uom> + lit<")">)> {};
  struct Power
      : rule<ref<Atom> +
             option(nest<ast::PowerUom, &ast::PowerUom::base>(
                 (lit<"^"> |
                  lit<"**">)+assign<&ast::PowerUom::exponent>(ref<INTE>)))> {};
  struct Uom : infix<Power, &ast::BinaryUom::left, &ast::BinaryUom::op,
                     &ast::BinaryUom::right, left(ref<MUL> | ref<DIV>)> {};

  using entry = Uom;
  using ignore = WS;
};

} // namespace metric
