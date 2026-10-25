// The checks of this language: what the grammar accepts but the language
// does not. None yet -- the grammar's own structure covers what parses.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "algol60/core/ast.hpp"

namespace algol60 {

class Algol60Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct Algol60CoreServices;
void register_validation_checks(Algol60CoreServices& services);

}  // namespace algol60
