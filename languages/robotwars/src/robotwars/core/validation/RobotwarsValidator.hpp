// The checks of this language: what the grammar accepts but the language
// does not. One here -- a fact no single node can see on its own.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "robotwars/core/ast.hpp"

namespace robotwars {

class RobotwarsValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkUniqueLabels(const ast::Program& program,
                         cursiv::ValidationAcceptor& accept) const;
};

struct RobotwarsCoreServices;
void register_validation_checks(RobotwarsCoreServices& services);

}  // namespace robotwars
