#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "tl/core/ast.hpp"

namespace tl {

class TlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkMissing(const ast::Missing&, cursiv::ValidationAcceptor&) const;
};

struct TlCoreServices;
void register_validation_checks(TlCoreServices& services);

}  // namespace tl
