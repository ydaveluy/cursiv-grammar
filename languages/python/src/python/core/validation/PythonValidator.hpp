// The checks of this language: none yet -- the port covers parsing and the
// AST, not semantic validation.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "python/core/ast.hpp"

namespace python {

class PythonValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct PythonCoreServices;
void register_validation_checks(PythonCoreServices& services);

}  // namespace python
