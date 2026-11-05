#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "rust/ast.hpp"

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
