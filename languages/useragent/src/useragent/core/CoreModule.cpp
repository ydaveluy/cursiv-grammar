// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "useragent/core/CoreServices.hpp"
#include "useragent/core/validation/UseragentValidator.hpp"
#include "useragent/generated/Useragent/Module.hpp"

namespace useragent {

void install_core_module(UseragentCoreServices& services) {
  useragent::generated::Useragent::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace useragent
