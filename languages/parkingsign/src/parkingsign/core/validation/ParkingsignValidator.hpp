// The checks of this language: none yet -- nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "parkingsign/core/ast.hpp"

namespace parkingsign {

class ParkingsignValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ParkingsignCoreServices;
void register_validation_checks(ParkingsignCoreServices& services);

}  // namespace parkingsign
