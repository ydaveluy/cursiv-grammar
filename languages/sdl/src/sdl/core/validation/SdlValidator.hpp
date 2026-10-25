// The checks of this language: none yet.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "sdl/core/ast.hpp"

namespace sdl {

class SdlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct SdlCoreServices;
void register_validation_checks(SdlCoreServices& services);

}  // namespace sdl
