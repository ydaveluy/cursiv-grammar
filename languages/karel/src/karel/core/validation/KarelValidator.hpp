// The checks of this language: what the grammar accepts but the language does
// not. One here -- a fact no single node can see on its own.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "karel/core/ast.hpp"

namespace karel {

class KarelValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkUniqueDefinitions(const ast::Program& program,
                              cursiv::ValidationAcceptor& accept) const;
};

struct KarelCoreServices;
void register_validation_checks(KarelCoreServices& services);

}  // namespace karel
