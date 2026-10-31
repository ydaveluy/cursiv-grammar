// The quakemap language: its grammar. NUMBER, STRING and TEXT are used
// directly wherever a number, a string or a name appears; no pass-through
// rule wraps them.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "quakemap/core/ast.hpp"

namespace quakemap {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
inline constexpr auto number = [](std::string_view t) {
  if (t.starts_with('"')) t = t.substr(1, t.size() - 2);
  return cursiv::default_convert<double>(t);
};

struct Quakemap : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"(\x01-\x20)">)> {};
  struct HWS : terminal<some(cr<R"(\x01-\x09\x0b-\x20)">)> {};
  struct COMMENT : terminal<lit<"//"> + many(cr<"^\n">)> {};
  static constexpr auto token_end = &(cr<R"(\x01-\x20)"> | eof);

  struct TEXT : terminal<!lit<"//"> + !lit<"\""> +
      some(cr<R"(^\x00-\x20)">)> {};

  static constexpr auto numeric = option(cr<"+-">) +
      ((some(d) + option(lit<"."> + many(d))) | (lit<"."> + some(d))) +
      option(cr<"eE"> + option(cr<"+-">) + some(d));
  struct NUMBER : terminal<as<number>((numeric + token_end) |
      (lit<"\""> + numeric + lit<"\"">))> {};

  struct STRING
      : terminal<as<unquote>(lit<"\""> + many(cr<R"(^\x00")">) +
                             lit<"\"">)> {};

  struct KeyValue
      : rule<skip(ignore<HWS>, hide<>,
             assign<&ast::KeyValue::key>(ref<STRING> | ref<TEXT>) +
             assign<&ast::KeyValue::value>(ref<STRING> | ref<TEXT>))> {};

  struct Coord
      : rule<skip(ignore<HWS>, hide<>,
             lit<"(", token_end> + repeat<3>(append<&ast::Coord::values>(ref<NUMBER>)) +
             lit<")", token_end>)> {};

  struct BrushLine
      : rule<repeat<2>(append<&ast::BrushLine::coords>(ref<Coord>)) +
             skip(ignore<HWS>, hide<>,
             append<&ast::BrushLine::coords>(ref<Coord>) +
             assign<&ast::BrushLine::texture>(ref<TEXT> | ref<STRING>) +
             repeat<5>(append<&ast::BrushLine::values>(ref<NUMBER>)))> {};

  struct Brush
      : rule<lit<"{", token_end> + many(append<&ast::Brush::lines>(ref<BrushLine>)) +
             lit<"}", token_end>> {};

  struct Entity
      : rule<lit<"{", token_end> +
             many(append<&ast::Entity::keyvalues>(ref<KeyValue>) |
                  append<&ast::Entity::brushes>(ref<Brush>)) + lit<"}", token_end>> {
  };

  struct Map
      : rule<create<ast::Map> +
             many(append<&ast::Map::entities>(ref<Entity>))> {};

  using entry = Map;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace quakemap
