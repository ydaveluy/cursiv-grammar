// The checks of this language: none yet -- nothing here needs a check
// beyond parsing.
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
