// Hook for runtime checks. Name use, bounds and reversibility are not yet
// validated here; syntax alone does not establish reversible execution.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "janus/core/ast.hpp"

namespace janus {

class JanusValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct JanusCoreServices;
void register_validation_checks(JanusCoreServices& services);

}  // namespace janus
