// The checks of this language: what the grammar accepts but the language
// does not. None yet -- nothing here needs a check beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "r/core/ast.hpp"

namespace r {

class RValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct RCoreServices;
void register_validation_checks(RCoreServices& services);

}  // namespace r
