// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "hypertalk/core/CoreServices.hpp"
#include "hypertalk/core/validation/HypertalkValidator.hpp"
#include "hypertalk/generated/Hypertalk/Module.hpp"

namespace hypertalk {

void install_core_module(HypertalkCoreServices& services) {
  hypertalk::generated::Hypertalk::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace hypertalk
