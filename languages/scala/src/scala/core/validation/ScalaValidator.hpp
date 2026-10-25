// The checks of this language. None yet: the grammar is syntactic only, and
// nothing here needs a check the parser itself can't already enforce.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "scala/core/ast.hpp"

namespace scala {

class ScalaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ScalaCoreServices;
void register_validation_checks(ScalaCoreServices& services);

}  // namespace scala
