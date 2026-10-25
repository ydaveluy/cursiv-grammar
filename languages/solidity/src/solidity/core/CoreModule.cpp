// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "solidity/core/CoreServices.hpp"
#include "solidity/core/validation/SolidityValidator.hpp"
#include "solidity/generated/Solidity/Module.hpp"

namespace solidity {

void install_core_module(SolidityCoreServices& services) {
  solidity::generated::Solidity::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace solidity
