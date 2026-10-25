#pragma once

#include "cursiv/core/references/DefaultScopeComputation.hpp"

// A label has whole-function scope, not block scope: a `goto` may cross into
// or out of a nested `{ }` that a plain container walk would hide it behind.

namespace alef {

class AlefScopeComputation final : public cursiv::DefaultScopeComputation {
 public:
  using cursiv::DefaultScopeComputation::DefaultScopeComputation;

 protected:
  void addLocalSymbol(cursiv::AstNode& node, const cursiv::Document& document,
                      cursiv::LocalSymbols& symbols) const override;
};

}  // namespace alef
