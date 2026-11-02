// Local checks for V's exceptional interpolation delimiter tolerance.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "v/core/ast.hpp"

namespace v {

class VValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkInterpolation(const ast::StringInterpolation& interpolation,
                          cursiv::ValidationAcceptor& accept) const;
  void checkUnsafe(const ast::UnsafeExpr& expression,
                   cursiv::ValidationAcceptor& accept) const;
};

struct VCoreServices;
void register_validation_checks(VCoreServices& services);

}  // namespace v
