// The checks of this language: none. The grammar permits keywords as names
// and generic I/O and attribute statements; no invariant here needs a
// semantic pass to catch.
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
