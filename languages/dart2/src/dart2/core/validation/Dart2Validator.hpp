// The checks of this language: what the grammar accepts but the language
// does not. None yet.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "dart2/core/ast.hpp"

namespace dart2 {

class Dart2Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct Dart2CoreServices;
void register_validation_checks(Dart2CoreServices& services);

}  // namespace dart2
