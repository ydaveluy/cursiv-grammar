// The checks of this language: none yet, syntax is all this port judges.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "bicep/ast.hpp"

namespace bicep {

class BicepValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct BicepCoreServices;
void register_validation_checks(BicepCoreServices& services);

}  // namespace bicep
