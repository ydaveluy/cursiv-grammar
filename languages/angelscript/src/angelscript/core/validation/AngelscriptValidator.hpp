// The checks of this language: none yet.
#pragma once

#include "angelscript/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace angelscript {

class AngelscriptValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct AngelscriptCoreServices;
void register_validation_checks(AngelscriptCoreServices& services);

}  // namespace angelscript
