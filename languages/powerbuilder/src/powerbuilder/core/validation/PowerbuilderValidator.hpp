// The checks of this language. None yet: the grammar carries the language's
// structure and nothing here needs a check beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "powerbuilder/core/ast.hpp"

namespace powerbuilder {

class PowerbuilderValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct PowerbuilderCoreServices;
void register_validation_checks(PowerbuilderCoreServices& services);

}  // namespace powerbuilder
