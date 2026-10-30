// The databank language: its grammar.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "databank/core/ast.hpp"

namespace databank {

inline constexpr auto na_value = [](std::string_view) {
  return std::monostate{};
};

struct Databank : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};

  struct EOL : terminal<some(cr<R"(\r\n)">)> {};

  struct COMMENT
      : terminal<lit<"\"c"> + many(cr<R"(^")">) + lit<"\""> + many(lit<" ">) +
                 some(cr<R"(\r\n)">)> {};

  struct FLOATINGPOINT
      : terminal<as<double>(option(cr<"+-">) + some(d) +
                            option(lit<"."> + many(d)) +
                            option(cr<"eE"> + option(cr<"+-">) + some(d)))> {};

  // kw<>, not lit<>: the word boundary keeps -1 off the lead digit of -12.
  struct DatatypeCode : terminal<as<double>(kw<"-1"> | kw<"-4"> | kw<"-12">)> {
  };

  struct NA : terminal<as<na_value>(kw<"NA">)> {};

  static constexpr auto sample_value = ref<FLOATINGPOINT> | ref<NA>;

  struct Series
      : rule<many(ref<EOL>) +
             option(assign<&ast::Series::datatype>(ref<DatatypeCode>) +
                    ref<EOL>) +
             assign<&ast::Series::from>(ref<FLOATINGPOINT>) + ref<EOL> +
             assign<&ast::Series::to>(ref<FLOATINGPOINT>) + ref<EOL> +
             some(append<&ast::Series::samples>(sample_value) +
                  (ref<EOL> | eof)) +
             many(ref<EOL>)> {};

  using entry = Series;
  using ignored = list<WS, COMMENT>;
};

}  // namespace databank
