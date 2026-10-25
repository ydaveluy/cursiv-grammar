// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "maml/core/CoreServices.hpp"
#include "maml/core/validation/MamlValidator.hpp"
#include "maml/generated/Maml/Module.hpp"

namespace maml {

void install_core_module(MamlCoreServices& services) {
  maml::generated::Maml::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace maml
