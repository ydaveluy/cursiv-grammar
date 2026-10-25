#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "gtin/core/ast.hpp"
namespace gtin {
struct Gtin : cursiv::GrammarSpec {
  struct CODE : terminal<some(any)> {};
  struct GtinRule
      : rule<create<ast::Gtin> + option(assign<&ast::Gtin::text>(ref<CODE>))> {
  };
  using entry = GtinRule;
};
}  // namespace gtin
