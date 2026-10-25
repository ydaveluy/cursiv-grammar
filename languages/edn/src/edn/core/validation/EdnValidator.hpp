// The checks of this language: what the grammar accepts but the language does
// not. edn has no such rule -- it is a data notation, not a program -- so
// the pair stays here for the language container to wire up, empty.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "edn/core/ast.hpp"

namespace edn {

class EdnValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct EdnCoreServices;
void register_validation_checks(EdnCoreServices& services);

}  // namespace edn
