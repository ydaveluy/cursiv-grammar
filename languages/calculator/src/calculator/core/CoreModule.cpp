// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "calculator/core/CoreServices.hpp"
#include "calculator/core/validation/CalculatorValidator.hpp"
#include "calculator/generated/Calculator/Module.hpp"

namespace calculator {

void install_core_module(CalculatorCoreServices& services) {
  calculator::generated::Calculator::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace calculator
