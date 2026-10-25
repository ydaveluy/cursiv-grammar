// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "gql/core/CoreServices.hpp"
#include "gql/core/validation/GqlValidator.hpp"
#include "gql/generated/Gql/Module.hpp"

namespace gql {

void install_core_module(GqlCoreServices& services) {
  gql::generated::Gql::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace gql
