// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "fusiontables/core/CoreServices.hpp"
#include "fusiontables/core/validation/FusionTablesValidator.hpp"
#include "fusiontables/generated/FusionTables/Module.hpp"

namespace fusiontables {

void install_core_module(FusionTablesCoreServices& services) {
  fusiontables::generated::FusionTables::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace fusiontables
