// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "alpaca/core/CoreServices.hpp"
#include "alpaca/core/validation/AlpacaValidator.hpp"
#include "alpaca/generated/Alpaca/Module.hpp"

namespace alpaca {

void install_core_module(AlpacaCoreServices& services) {
  alpaca::generated::Alpaca::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace alpaca
