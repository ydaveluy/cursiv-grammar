// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "graphstreamdgs/core/CoreServices.hpp"
#include "graphstreamdgs/core/validation/GraphstreamDgsValidator.hpp"
#include "graphstreamdgs/generated/GraphstreamDgs/Module.hpp"

namespace graphstreamdgs {

void install_core_module(GraphstreamDgsCoreServices& services) {
  graphstreamdgs::generated::GraphstreamDgs::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace graphstreamdgs
