// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "guido/core/CoreServices.hpp"
#include "guido/core/validation/GuidoValidator.hpp"
#include "guido/generated/Guido/Module.hpp"

namespace guido {

void install_core_module(GuidoCoreServices& services) {
  guido::generated::Guido::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace guido
