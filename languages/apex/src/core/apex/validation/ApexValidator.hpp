// The checks of this language: what the grammar accepts but the language
// does not. None yet -- nothing here needs a check beyond parsing.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "apex/ast.hpp"

namespace apex {

class ApexValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ApexCoreServices;
void register_validation_checks(ApexCoreServices& services);

}  // namespace apex
