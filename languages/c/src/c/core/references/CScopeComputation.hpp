#pragma once

#include "cursiv/core/references/DefaultScopeComputation.hpp"

// A label has function scope in C, not block scope: a `goto` may cross into
// or out of a nested `{ }` that a plain container walk would hide it behind.

namespace c {

class CScopeComputation final : public cursiv::DefaultScopeComputation {
 public:
  using cursiv::DefaultScopeComputation::DefaultScopeComputation;

 protected:
  void addLocalSymbol(cursiv::AstNode& node, const cursiv::Document& document,
                      cursiv::LocalSymbols& symbols) const override;
};

}  // namespace c
