// The checks of this language: what the grammar accepts but the language
// does not: label constraints, numeric range and duplicate declarations.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "robotwars/core/ast.hpp"

namespace robotwars {

class RobotwarsValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkNumber(const ast::NumberLit& number, cursiv::ValidationAcceptor& accept) const;
  void checkLabel(const ast::Label& label, cursiv::ValidationAcceptor& accept) const;

  void checkUniqueLabels(const ast::Program& program,
                         cursiv::ValidationAcceptor& accept) const;
};

struct RobotwarsCoreServices;
void register_validation_checks(RobotwarsCoreServices& services);

}  // namespace robotwars
