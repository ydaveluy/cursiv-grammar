// The itn language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "itn/core/ast.hpp"

namespace itn {

struct Itn : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\v\f)">)> {};
  struct NEWLINE : terminal<eol> {};
  struct META : terminal<lit<"~"> + many(cr<R"(^\r\n)">)> {};
  struct BOM : terminal<lit<"\ufeff"> | lit<"ï»¿">> {};
  struct NAME : terminal<lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">> {};

  struct NUM : terminal<some(d)> {};
  struct COORD : terminal<as<std::int64_t>(option(cr<"+-">) + some(d))> {};
  struct TEXT : terminal<some(cr<R"(^|\r\n)">)> {};
  static constexpr auto LineEnd = option(ref<WS>) + (ref<NEWLINE> | eof);

  struct Line
      : rule<option(ref<WS>) +
             assign<&ast::Line::longitude>(ref<COORD>) + lit<"|"> + option(ref<WS>) +
             assign<&ast::Line::latitude>(ref<COORD>) + lit<"|"> +
             option(assign<&ast::Line::descr>(ref<TEXT>)) + lit<"|"> +
             assign<&ast::Line::flag>(ref<NUM>) + lit<"|"> + LineEnd> {};

  struct BlankLine : rule<option(ref<WS>) + ref<NEWLINE>> {};

  struct Itinerary
      : rule<create<ast::Itinerary> + many(
             ref<BlankLine> | ref<META> | (option(ref<BOM>) +
             ((append<&ast::Itinerary::names>(ref<NAME>) + LineEnd) |
              append<&ast::Itinerary::lines>(ref<Line>)))) + option(ref<WS>)> {};

  using entry = Itinerary;
};

}  // namespace itn
