// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "spl/core/CoreServices.hpp"
#include "spl/core/validation/SplValidator.hpp"
#include "spl/generated/Spl/Module.hpp"

namespace spl {

void install_core_module(SplCoreServices& services) {
  spl::generated::Spl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace spl
