// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "iri/core/CoreServices.hpp"
#include "iri/core/validation/IriValidator.hpp"
#include "iri/generated/Iri/Module.hpp"

namespace iri {

void install_core_module(IriCoreServices& services) {
  iri::generated::Iri::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace iri
