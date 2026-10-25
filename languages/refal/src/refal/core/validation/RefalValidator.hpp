// The checks of this language. Empty for now: nothing the grammar accepts
// needs a further, tree-wide rejection.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "refal/core/ast.hpp"

namespace refal {

class RefalValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct RefalCoreServices;
void register_validation_checks(RefalCoreServices& services);

}  // namespace refal
