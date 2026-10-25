// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "kquery/core/CoreServices.hpp"
#include "kquery/core/validation/KqueryValidator.hpp"
#include "kquery/generated/Kquery/Module.hpp"

namespace kquery {

void install_core_module(KqueryCoreServices& services) {
  kquery::generated::Kquery::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace kquery
