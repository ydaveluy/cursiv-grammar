// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "aspectj/core/CoreServices.hpp"
#include "aspectj/core/validation/AspectjValidator.hpp"
#include "aspectj/generated/Aspectj/Module.hpp"

namespace aspectj {

void install_core_module(AspectjCoreServices& services) {
  aspectj::generated::Aspectj::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace aspectj
