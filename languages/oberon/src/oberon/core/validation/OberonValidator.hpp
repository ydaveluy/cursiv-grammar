// The checks of this language. None yet: the port has no symbol table, so
// there is nothing a single node -- or the whole tree -- can judge.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "oberon/core/ast.hpp"

namespace oberon {

class OberonValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct OberonCoreServices;
void register_validation_checks(OberonCoreServices& services);

}  // namespace oberon
