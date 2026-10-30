// Scope, database-dependent names and execution constraints are not yet
// validated by this port.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "aql/core/ast.hpp"

namespace aql {

class AqlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct AqlCoreServices;
void register_validation_checks(AqlCoreServices& services);

}  // namespace aql
