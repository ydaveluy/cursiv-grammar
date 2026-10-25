// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "cql3/core/CoreServices.hpp"
#include "cql3/core/validation/Cql3Validator.hpp"
#include "cql3/generated/Cql3/Module.hpp"

namespace cql3 {

void install_core_module(Cql3CoreServices& services) {
  cql3::generated::Cql3::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace cql3
