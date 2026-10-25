// The gml language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "gml/core/ast.hpp"
#include "gml/core/StringValue.hpp"

namespace gml {

struct Gml : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  // The key side of a key/value pair never competes with a number, so it
  // keeps an unrestricted alnum run, digits included.
  struct KEY : terminal<some(cr<"a-zA-Z0-9_">)> {};

  static constexpr auto fraction = (many(d) + lit<"."> + some(d)) |
                                    (some(d) + lit<"."> + many(d));
  struct NUMBER : terminal<option(cr<"+-">) +
      ((fraction + option(cr<"eE"> + option(cr<"+-">) + some(d))) |
       some(d) | lit<"INF"> | lit<"NAN">)> {};
  struct NumberRule : rule<assign<&ast::Number::literal>(ref<NUMBER>)> {};

  struct STRINGLITERAL
      : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">,
                 text_conv<decode_string>> {};

  // A bare word, on the value side: a run of digits alone lexes as INTEGER
  // instead, so the two never tie on the same text.
  struct WORD : terminal<many(d) + cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">)> {};

  struct ListRule;

  static constexpr auto value = ref<NumberRule> | ref<WORD> | ref<ListRule>;

  struct KvRule : rule<assign<&ast::Kv::key>(ref<KEY>) +
                       (flag<&ast::Kv::quoted>(
                            assign<&ast::Kv::value>(ref<STRINGLITERAL>)) |
                        assign<&ast::Kv::value>(value))> {};

  struct GraphRule : rule<many(append<&ast::List::entries>(ref<KvRule>))> {};

  struct ListRule : rule<lit<"["> +
                         many(append<&ast::List::entries>(ref<KvRule>)) +
                         lit<"]">> {};

  using entry = GraphRule;
  using ignore = list<WS, COMMENT>;
};

}  // namespace gml
