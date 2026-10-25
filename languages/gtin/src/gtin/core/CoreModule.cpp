// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "gtin/core/CoreServices.hpp"
#include "gtin/core/validation/GtinValidator.hpp"
#include "gtin/generated/Gtin/Module.hpp"

namespace gtin {

void install_core_module(GtinCoreServices& services) {
  gtin::generated::Gtin::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace gtin
