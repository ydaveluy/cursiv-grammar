// The checks of this language: none yet -- nothing here needs a check
// beyond parsing.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "chip8/ast.hpp"

namespace chip8 {

class Chip8Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct Chip8CoreServices;
void register_validation_checks(Chip8CoreServices& services);

}  // namespace chip8
