// The checks of this language: what the grammar accepts but the language does
// not. None yet -- the pair stays so a check has somewhere to go.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "trapc/ast.hpp"

namespace trapc {

class TrapcValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct TrapcCoreServices;
void register_validation_checks(TrapcCoreServices& services);

}  // namespace trapc
