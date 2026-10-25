// The checks of this language: what the grammar accepts but the language
// does not. None yet -- Cypher's parseable surface needs no check beyond
// what the grammar itself enforces.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "cypher/core/ast.hpp"

namespace cypher {

class CypherValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct CypherCoreServices;
void register_validation_checks(CypherCoreServices& services);

}  // namespace cypher
