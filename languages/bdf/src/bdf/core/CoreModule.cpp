// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "bdf/core/CoreServices.hpp"
#include "bdf/core/validation/BdfValidator.hpp"
#include "bdf/generated/Bdf/Module.hpp"

namespace bdf {

void install_core_module(BdfCoreServices& services) {
  bdf::generated::Bdf::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace bdf
