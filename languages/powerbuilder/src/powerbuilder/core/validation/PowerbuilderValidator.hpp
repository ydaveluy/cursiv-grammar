// Numeric literals retain their source spelling; range errors are reported
// without rounding the integer or decimal value stored in the tree.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "powerbuilder/core/ast.hpp"

namespace powerbuilder {

class PowerbuilderValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkNumber(const ast::NumberLit&, cursiv::ValidationAcceptor&) const;
};

struct PowerbuilderCoreServices;
void register_validation_checks(PowerbuilderCoreServices& services);

}  // namespace powerbuilder
