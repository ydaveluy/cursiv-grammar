// Local tuple-name checks; model typing and evaluation remain separate.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "ocl/core/ast.hpp"

namespace ocl {

class OclValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkTupleLiteral(const ast::TupleLiteral&, cursiv::ValidationAcceptor&) const;
  void checkTupleType(const ast::TupleType&, cursiv::ValidationAcceptor&) const;
};

struct OclCoreServices;
void register_validation_checks(OclCoreServices& services);

}  // namespace ocl
