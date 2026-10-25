// The checks of this language: none yet. Nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "vb6/core/ast.hpp"

namespace vb6 {

class Vb6Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct Vb6CoreServices;
void register_validation_checks(Vb6CoreServices& services);

}  // namespace vb6
