// The checks of this language: none yet -- nothing here needs a check
// beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "chip8/core/ast.hpp"

namespace chip8 {

class Chip8Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct Chip8CoreServices;
void register_validation_checks(Chip8CoreServices& services);

}  // namespace chip8
