// Numeric constraints which depend on a token's declared radix.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "clojure/ast.hpp"

namespace clojure {

class ClojureValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkRadix(const ast::RadixLit&, cursiv::ValidationAcceptor&) const;
};

struct ClojureCoreServices;
void register_validation_checks(ClojureCoreServices& services);

}  // namespace clojure
