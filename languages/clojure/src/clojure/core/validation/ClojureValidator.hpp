// The checks of this language. None yet: nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "clojure/core/ast.hpp"

namespace clojure {

class ClojureValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ClojureCoreServices;
void register_validation_checks(ClojureCoreServices& services);

}  // namespace clojure
