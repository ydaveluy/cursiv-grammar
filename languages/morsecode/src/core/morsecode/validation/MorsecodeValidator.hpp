// The checks of this language: none -- every letter is a fixed dot/dash
// pattern, so the grammar already rejects everything a check would flag.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "morsecode/ast.hpp"

namespace morsecode {

class MorsecodeValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct MorsecodeCoreServices;
void register_validation_checks(MorsecodeCoreServices& services);

}  // namespace morsecode
