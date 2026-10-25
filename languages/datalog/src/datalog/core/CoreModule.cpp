// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "datalog/core/CoreServices.hpp"
#include "datalog/core/validation/DatalogValidator.hpp"
#include "datalog/generated/Datalog/Module.hpp"

namespace datalog {

void install_core_module(DatalogCoreServices& services) {
  datalog::generated::Datalog::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace datalog
