// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "aql/core/CoreServices.hpp"
#include "aql/core/validation/AqlValidator.hpp"
#include "aql/generated/Aql/Module.hpp"

namespace aql {

void install_core_module(AqlCoreServices& services) {
  aql::generated::Aql::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace aql
