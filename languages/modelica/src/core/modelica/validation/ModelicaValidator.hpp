// The checks of this language: syntax-only for now, no semantic check
// registered.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "modelica/ast.hpp"

namespace modelica {

class ModelicaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ModelicaCoreServices;
void register_validation_checks(ModelicaCoreServices& services);

}  // namespace modelica
