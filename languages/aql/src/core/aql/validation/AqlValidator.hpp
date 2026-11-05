// Scope, database-dependent names and execution constraints are not yet
// validated by this port.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "aql/ast.hpp"

namespace aql {

class AqlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct AqlCoreServices;
void register_validation_checks(AqlCoreServices& services);

}  // namespace aql
