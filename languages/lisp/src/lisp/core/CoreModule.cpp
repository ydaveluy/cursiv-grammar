// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "lisp/core/CoreServices.hpp"
#include "lisp/core/validation/LispValidator.hpp"
#include "lisp/generated/Lisp/Module.hpp"

namespace lisp {

void install_core_module(LispCoreServices& services) {
  lisp::generated::Lisp::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace lisp
