// The checks of this language: what the grammar accepts but the language does
// not -- one here, the fact no single node can see on its own.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "cto/ast.hpp"

namespace cto {

class CtoValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkUniqueDeclarations(const ast::Model& model,
                               cursiv::ValidationAcceptor& accept) const;
};

struct CtoCoreServices;
void register_validation_checks(CtoCoreServices& services);

}  // namespace cto
