// The quakemap language: its grammar. NUMBER, STRING and TEXT are used
// directly wherever a number, a string or a name appears; no pass-through
// rule wraps them.
#pragma once

#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "quakemap/core/ast.hpp"

namespace quakemap {

inline constexpr auto unquote = [](std::string_view t,
                                   std::pmr::memory_resource* mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('"') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[n++] = body[i];
    if (body[i] == '"') ++i;
  }
  return std::string_view(room.data(), n);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Quakemap : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct TEXT : terminal<cr<"a-zA-Z"> + some(w)> {};

  struct NUMBER
      : terminal<option(cr<"+-">) + some(d) + option(lit<"."> + some(d)),
                 double> {};

  struct STRING
      : terminal<lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">,
                 Unquoted> {};

  struct KeyValueRule
      : rule<assign<&ast::KeyValue::key>(ref<STRING>) +
             assign<&ast::KeyValue::value>(ref<STRING>)> {};

  struct CoordRule
      : rule<lit<"("> + many(append<&ast::Coord::values>(ref<NUMBER>)) +
             lit<")">> {};

  struct BrushLineRule
      : rule<many(append<&ast::BrushLine::coords>(ref<CoordRule>)) +
             assign<&ast::BrushLine::texture>(ref<TEXT>) +
             many(append<&ast::BrushLine::values>(ref<NUMBER>))> {};

  struct BrushRule
      : rule<lit<"{"> + some(append<&ast::Brush::lines>(ref<BrushLineRule>)) +
             lit<"}">> {};

  struct EntityRule
      : rule<lit<"{"> +
             many(append<&ast::Entity::keyvalues>(ref<KeyValueRule>)) +
             many(append<&ast::Entity::brushes>(ref<BrushRule>)) + lit<"}">> {
  };

  struct MapRule
      : rule<create<ast::Map> +
             many(append<&ast::Map::entities>(ref<EntityRule>))> {};

  using entry = MapRule;
  using ignore = WS;
};

}  // namespace quakemap
