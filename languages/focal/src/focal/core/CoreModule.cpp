// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "focal/core/CoreServices.hpp"
#include "focal/core/validation/FocalValidator.hpp"
#include "focal/generated/Focal/Module.hpp"

namespace focal {

void install_core_module(FocalCoreServices& services) {
  focal::generated::Focal::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace focal
