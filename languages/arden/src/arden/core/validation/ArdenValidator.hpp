// The checks of this language: what the grammar accepts but the language does
// not. None yet.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "arden/core/ast.hpp"

namespace arden {

class ArdenValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ArdenCoreServices;
void register_validation_checks(ArdenCoreServices& services);

}  // namespace arden
