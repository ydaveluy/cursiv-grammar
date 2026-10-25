// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "clf/core/CoreServices.hpp"
#include "clf/core/validation/ClfValidator.hpp"
#include "clf/generated/Clf/Module.hpp"

namespace clf {

void install_core_module(ClfCoreServices& services) {
  clf::generated::Clf::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace clf
