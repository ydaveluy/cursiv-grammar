// The checks of this language: what the grammar accepts but the language
// does not. None yet -- the grammar's shape already carries what matters.
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
