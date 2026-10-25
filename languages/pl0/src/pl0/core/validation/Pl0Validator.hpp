// The checks of this language: none yet -- the grammar carries no rule a
// single node, or the whole tree, needs to police beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "pl0/core/ast.hpp"

namespace pl0 {

class Pl0Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct Pl0CoreServices;
void register_validation_checks(Pl0CoreServices& services);

}  // namespace pl0
