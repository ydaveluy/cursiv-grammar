// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "yini/core/CoreServices.hpp"
#include "yini/core/validation/YiniValidator.hpp"
#include "yini/generated/Yini/Module.hpp"

namespace yini {

void install_core_module(YiniCoreServices& services) {
  yini::generated::Yini::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace yini
