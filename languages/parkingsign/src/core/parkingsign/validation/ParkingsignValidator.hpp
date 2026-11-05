// The checks of this language: none yet -- nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "parkingsign/ast.hpp"

namespace parkingsign {

class ParkingsignValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ParkingsignCoreServices;
void register_validation_checks(ParkingsignCoreServices& services);

}  // namespace parkingsign
