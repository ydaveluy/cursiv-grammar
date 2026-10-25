// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "gedcom/core/CoreServices.hpp"
#include "gedcom/core/validation/GedcomValidator.hpp"
#include "gedcom/generated/Gedcom/Module.hpp"

namespace gedcom {

void install_core_module(GedcomCoreServices& services) {
  gedcom::generated::Gedcom::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace gedcom
