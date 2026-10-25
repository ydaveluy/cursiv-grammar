// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "arithmetic/core/CoreServices.hpp"
#include "arithmetic/core/validation/ArithmeticValidator.hpp"
#include "arithmetic/generated/Arithmetic/Module.hpp"

namespace arithmetic {

void install_core_module(ArithmeticCoreServices& services) {
  arithmetic::generated::Arithmetic::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace arithmetic
