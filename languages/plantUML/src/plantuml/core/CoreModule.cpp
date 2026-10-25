// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "plantuml/core/CoreServices.hpp"
#include "plantuml/core/validation/PlantUMLValidator.hpp"
#include "plantuml/generated/PlantUML/Module.hpp"

namespace plantuml {

void install_core_module(PlantUMLCoreServices& services) {
  plantuml::generated::PlantUML::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace plantuml
