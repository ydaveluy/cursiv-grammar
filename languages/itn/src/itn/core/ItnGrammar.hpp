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
  struct COORD : terminal<option(cr<"+-">) + some(d), std::int64_t> {};
  static constexpr auto suffix = lit<"|"> + some(d) + option(lit<"|">) +
      many(cr<R"( \t\v\f)">) + (&eol | eof);
  struct TEXT : terminal<some(!suffix + cr<R"(^\r\n)">)> {};

  struct LineRule
      : rule<option(ref<WS>) +
             assign<&ast::Line::longitude>(ref<COORD>) + lit<"|"> + option(ref<WS>) +
             assign<&ast::Line::latitude>(ref<COORD>) + lit<"|"> +
             option(assign<&ast::Line::descr>(ref<TEXT>)) + lit<"|"> +
             assign<&ast::Line::flag>(ref<NUM>) + option(lit<"|">) + option(ref<WS>)> {};

  struct BlankLine : rule<option(ref<WS>) + ref<NEWLINE>> {};

  struct ItineraryRule
      : rule<create<ast::Itinerary> + many(
             ref<BlankLine> | ref<META> | (option(ref<BOM>) +
             (append<&ast::Itinerary::names>(ref<NAME>) |
              append<&ast::Itinerary::lines>(ref<LineRule>)))) + option(ref<WS>)> {};

  using entry = ItineraryRule;
};

}  // namespace itn
