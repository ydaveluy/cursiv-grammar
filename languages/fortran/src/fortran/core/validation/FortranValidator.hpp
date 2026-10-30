// No semantic checks are registered yet. Declaration, type, DATA, I/O and
// construct constraints are outside this permissive syntax importer.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "fortran/core/ast.hpp"

namespace fortran {

class FortranValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct FortranCoreServices;
void register_validation_checks(FortranCoreServices& services);

}  // namespace fortran
