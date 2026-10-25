// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "sparql/core/CoreServices.hpp"
#include "sparql/core/validation/SparqlValidator.hpp"
#include "sparql/generated/Sparql/Module.hpp"

namespace sparql {

void install_core_module(SparqlCoreServices& services) {
  sparql::generated::Sparql::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace sparql
