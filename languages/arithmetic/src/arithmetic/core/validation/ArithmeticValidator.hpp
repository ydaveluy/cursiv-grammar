// The checks of this language. None yet: the grammar's shape is the language.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace arithmetic {

class ArithmeticValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ArithmeticCoreServices;
void register_validation_checks(ArithmeticCoreServices& services);

}  // namespace arithmetic
