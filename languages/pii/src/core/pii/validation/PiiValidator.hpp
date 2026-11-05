// The checks of this language: what the grammar accepts but the language
// does not. None yet: nothing here needs a check beyond parsing.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "pii/ast.hpp"

namespace pii {

class PiiValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct PiiCoreServices;
void register_validation_checks(PiiCoreServices& services);

}  // namespace pii
