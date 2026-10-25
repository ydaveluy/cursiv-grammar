// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "plucid/core/CoreServices.hpp"
#include "plucid/core/validation/PlucidValidator.hpp"
#include "plucid/generated/Plucid/Module.hpp"

namespace plucid {

void install_core_module(PlucidCoreServices& services) {
  plucid::generated::Plucid::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace plucid
