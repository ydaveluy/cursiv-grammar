#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "infosapient/ast.hpp"

namespace infosapient {

class InfosapientValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct InfosapientCoreServices;
void register_validation_checks(InfosapientCoreServices& services);

}  // namespace infosapient
