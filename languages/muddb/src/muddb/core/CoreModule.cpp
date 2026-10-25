// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "muddb/core/CoreServices.hpp"
#include "muddb/core/validation/MuddbValidator.hpp"
#include "muddb/generated/Muddb/Module.hpp"

namespace muddb {

void install_core_module(MuddbCoreServices& services) {
  muddb::generated::Muddb::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace muddb
