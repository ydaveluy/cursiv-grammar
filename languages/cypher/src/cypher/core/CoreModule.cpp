// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "cypher/core/CoreServices.hpp"
#include "cypher/core/validation/CypherValidator.hpp"
#include "cypher/generated/Cypher/Module.hpp"

namespace cypher {

void install_core_module(CypherCoreServices& services) {
  cypher::generated::Cypher::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace cypher
