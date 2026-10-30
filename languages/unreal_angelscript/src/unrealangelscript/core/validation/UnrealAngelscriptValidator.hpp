// This syntax subset does not register semantic validation checks.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "unrealangelscript/core/ast.hpp"

namespace unrealangelscript {

class UnrealAngelscriptValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct UnrealAngelscriptCoreServices;
void register_validation_checks(UnrealAngelscriptCoreServices& services);

}  // namespace unrealangelscript
