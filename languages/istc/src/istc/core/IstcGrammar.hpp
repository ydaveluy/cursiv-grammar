// The istc language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "istc/core/ast.hpp"

namespace istc {

struct Istc : cursiv::GrammarSpec {
  struct CHAR : terminal<cr<"A-F0-9">> {};
  struct SEP : terminal<lit<"-"> | lit<" ">> {};
  struct WS : terminal<some(cr<R"(\r\n\t)">)> {};

  struct RegistrationRule : rule<repeat<3>(ref<CHAR>)> {};
  struct YearRule : rule<repeat<4>(ref<CHAR>)> {};
  struct WorkRule : rule<repeat<8>(ref<CHAR>)> {};

  struct IstcRule
      : rule<create<ast::Istc> + lit<"ISTC"> + lit<" "> +
             assign<&ast::Istc::registration>(ref<RegistrationRule>) +
             ref<SEP> +
             assign<&ast::Istc::year>(ref<YearRule>) +
             ref<SEP> +
             assign<&ast::Istc::work>(ref<WorkRule>) +
             ref<SEP> +
             assign<&ast::Istc::check>(ref<CHAR>) +
             eof> {};

  using entry = IstcRule;
  using ignore = WS;
};

}  // namespace istc
