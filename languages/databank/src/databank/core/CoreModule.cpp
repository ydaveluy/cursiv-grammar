// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "databank/core/CoreServices.hpp"
#include "databank/core/validation/DatabankValidator.hpp"
#include "databank/generated/Databank/Module.hpp"

namespace databank {

void install_core_module(DatabankCoreServices& services) {
  databank::generated::Databank::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace databank
