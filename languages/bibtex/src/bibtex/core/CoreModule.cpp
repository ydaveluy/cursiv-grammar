// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "bibtex/core/CoreServices.hpp"
#include "bibtex/core/validation/BibtexValidator.hpp"
#include "bibtex/generated/Bibtex/Module.hpp"

namespace bibtex {

void install_core_module(BibtexCoreServices& services) {
  bibtex::generated::Bibtex::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace bibtex
