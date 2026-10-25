// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "smalltalk/core/CoreServices.hpp"
#include "smalltalk/core/validation/SmalltalkValidator.hpp"
#include "smalltalk/generated/Smalltalk/Module.hpp"

namespace smalltalk {

void install_core_module(SmalltalkCoreServices& services) {
  smalltalk::generated::Smalltalk::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace smalltalk
