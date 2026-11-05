// This syntax subset does not register semantic validation checks.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "unrealangelscript/ast.hpp"

namespace unrealangelscript {

class UnrealAngelscriptValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct UnrealAngelscriptCoreServices;
void register_validation_checks(UnrealAngelscriptCoreServices& services);

}  // namespace unrealangelscript
