// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "promql/core/CoreServices.hpp"
#include "promql/core/validation/PromqlValidator.hpp"
#include "promql/generated/Promql/Module.hpp"

namespace promql {

void install_core_module(PromqlCoreServices& services) {
  promql::generated::Promql::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace promql
