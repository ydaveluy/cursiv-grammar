// The istc language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "istc/core/ast.hpp"

namespace istc {

struct Istc : cursiv::GrammarSpec {
  struct CHAR : terminal<cr<"A-F0-9">> {};
  struct SEP : terminal<lit<"-"> | some(lit<" ">)> {};
  struct WS : terminal<some(cr<R"(\r\n\t)">)> {};

  struct Registration : terminal<repeat<3>(ref<CHAR>)> {};
  struct Year : terminal<repeat<4>(ref<CHAR>)> {};
  struct Work : terminal<repeat<8>(ref<CHAR>)> {};

  struct IstcRule
      : rule<create<ast::Istc> + kw<"ISTC"> + some(lit<" ">) +
             assign<&ast::Istc::registration>(ref<Registration>) +
             option(ref<SEP>) +
             assign<&ast::Istc::year>(ref<Year>) +
             option(ref<SEP>) +
             assign<&ast::Istc::work>(ref<Work>) +
             option(ref<SEP>) +
             assign<&ast::Istc::check>(ref<CHAR>) +
             eof> {};

  using entry = IstcRule;
  using ignored = WS;
};

}  // namespace istc
