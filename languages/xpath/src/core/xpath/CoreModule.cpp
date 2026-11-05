// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "xpath/CoreServices.hpp"
#include "xpath/validation/XpathValidator.hpp"
#include "xpath/generated/Xpath/Module.hpp"

namespace xpath {

void install_core_module(XpathCoreServices& services) {
  xpath::generated::Xpath::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace xpath
