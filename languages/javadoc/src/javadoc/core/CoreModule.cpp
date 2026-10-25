// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "javadoc/core/CoreServices.hpp"
#include "javadoc/core/validation/JavadocValidator.hpp"
#include "javadoc/generated/Javadoc/Module.hpp"

namespace javadoc {

void install_core_module(JavadocCoreServices& services) {
  javadoc::generated::Javadoc::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace javadoc
