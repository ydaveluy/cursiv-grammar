// The checks of this language. None yet: the grammar structure of a PlantUML
// class diagram carries no invariant a single-pass check needs to add.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "plantuml/core/ast.hpp"

namespace plantuml {

class PlantUMLValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct PlantUMLCoreServices;
void register_validation_checks(PlantUMLCoreServices& services);

}  // namespace plantuml
