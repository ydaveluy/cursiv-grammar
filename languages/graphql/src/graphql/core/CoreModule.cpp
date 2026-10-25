// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "graphql/core/CoreServices.hpp"
#include "graphql/core/validation/GraphqlValidator.hpp"
#include "graphql/generated/Graphql/Module.hpp"

namespace graphql {

void install_core_module(GraphqlCoreServices& services) {
  graphql::generated::Graphql::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace graphql
