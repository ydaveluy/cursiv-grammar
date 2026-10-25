// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "qif/core/CoreServices.hpp"
#include "qif/core/validation/QifValidator.hpp"
#include "qif/generated/Qif/Module.hpp"

namespace qif {

void install_core_module(QifCoreServices& services) {
  qif::generated::Qif::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace qif
