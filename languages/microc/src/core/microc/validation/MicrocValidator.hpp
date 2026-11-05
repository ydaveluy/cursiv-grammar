#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "microc/ast.hpp"

namespace microc {

class MicrocValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkIdent(const ast::Ident&, cursiv::ValidationAcceptor&) const;
};

struct MicrocCoreServices;
void register_validation_checks(MicrocCoreServices& services);

}  // namespace microc
