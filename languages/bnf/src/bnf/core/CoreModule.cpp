// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "bnf/core/CoreServices.hpp"
#include "bnf/core/validation/BnfValidator.hpp"
#include "bnf/generated/Bnf/Module.hpp"

namespace bnf {

void install_core_module(BnfCoreServices& services) {
  bnf::generated::Bnf::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace bnf
