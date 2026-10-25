#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "rust/core/ast.hpp"

namespace rust {

class RustValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkComparisonChain(const ast::BinaryExpr& expression,
                            cursiv::ValidationAcceptor& accept) const;
};

struct RustCoreServices;
void register_validation_checks(RustCoreServices& services);

}  // namespace rust
