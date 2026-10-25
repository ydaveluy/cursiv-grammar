// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "modelica/core/CoreServices.hpp"
#include "modelica/core/validation/ModelicaValidator.hpp"
#include "modelica/generated/Modelica/Module.hpp"

namespace modelica {

void install_core_module(ModelicaCoreServices& services) {
  modelica::generated::Modelica::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace modelica
