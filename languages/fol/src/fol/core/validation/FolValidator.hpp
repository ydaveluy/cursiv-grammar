// The checks of this language: what the grammar accepts but the language
// does not. None yet -- the grammar already carries every constraint FOL
// documents are subject to.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "fol/core/ast.hpp"

namespace fol {

class FolValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct FolCoreServices;
void register_validation_checks(FolCoreServices& services);

}  // namespace fol
