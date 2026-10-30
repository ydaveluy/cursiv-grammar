// This port recognizes the pedagogical grammars-v4 FOL notation. It does not
// check symbol arities, bound variables, satisfiability or logical validity.
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
