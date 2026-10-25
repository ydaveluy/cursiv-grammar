// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "javacc/core/CoreServices.hpp"
#include "javacc/core/validation/JavaccValidator.hpp"
#include "javacc/generated/Javacc/Module.hpp"

namespace javacc {

void install_core_module(JavaccCoreServices& services) {
  javacc::generated::Javacc::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace javacc
