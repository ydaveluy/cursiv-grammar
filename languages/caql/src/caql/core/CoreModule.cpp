// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "caql/core/CoreServices.hpp"
#include "caql/core/validation/CaqlValidator.hpp"
#include "caql/generated/Caql/Module.hpp"

namespace caql {

void install_core_module(CaqlCoreServices& services) {
  caql::generated::Caql::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace caql
