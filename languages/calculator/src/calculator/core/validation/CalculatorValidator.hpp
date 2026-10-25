// The checks of this language. None yet: nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace calculator {

class CalculatorValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct CalculatorCoreServices;
void register_validation_checks(CalculatorCoreServices& services);

}  // namespace calculator
