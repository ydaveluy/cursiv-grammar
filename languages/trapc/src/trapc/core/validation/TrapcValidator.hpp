// The checks of this language: what the grammar accepts but the language does
// not. None yet -- the pair stays so a check has somewhere to go.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "trapc/core/ast.hpp"

namespace trapc {

class TrapcValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct TrapcCoreServices;
void register_validation_checks(TrapcCoreServices& services);

}  // namespace trapc
