// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "ebnf/core/CoreServices.hpp"
#include "ebnf/core/validation/EbnfValidator.hpp"
#include "ebnf/generated/Ebnf/Module.hpp"

namespace ebnf {

void install_core_module(EbnfCoreServices& services) {
  ebnf::generated::Ebnf::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace ebnf
