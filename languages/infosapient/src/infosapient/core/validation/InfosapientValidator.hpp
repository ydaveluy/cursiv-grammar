#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "infosapient/core/ast.hpp"

namespace infosapient {

class InfosapientValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct InfosapientCoreServices;
void register_validation_checks(InfosapientCoreServices& services);

}  // namespace infosapient
