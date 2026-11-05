// This port recognizes the pedagogical grammars-v4 FOL notation. It does not
// check symbol arities, bound variables, satisfiability or logical validity.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "fol/ast.hpp"

namespace fol {

class FolValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct FolCoreServices;
void register_validation_checks(FolCoreServices& services);

}  // namespace fol
