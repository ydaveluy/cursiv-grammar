// Local closing-name and block-DO checks. Declaration, type, DATA and I/O
// semantics still need symbol/type analysis beyond this syntax subset.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "fortran/core/ast.hpp"

namespace fortran {

class FortranValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkUnit(const ast::ProgramUnit&, cursiv::ValidationAcceptor&) const;
  void checkStatement(const ast::Stmt&, cursiv::ValidationAcceptor&) const;
};

struct FortranCoreServices;
void register_validation_checks(FortranCoreServices& services);

}  // namespace fortran
