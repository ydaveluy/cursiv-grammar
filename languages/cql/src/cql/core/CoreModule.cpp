// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "cql/core/CoreServices.hpp"
#include "cql/core/validation/CqlValidator.hpp"
#include "cql/generated/Cql/Module.hpp"

namespace cql {

void install_core_module(CqlCoreServices& services) {
  cql::generated::Cql::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace cql
