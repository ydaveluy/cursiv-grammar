// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "webidl/core/CoreServices.hpp"
#include "webidl/core/validation/WebidlValidator.hpp"
#include "webidl/generated/Webidl/Module.hpp"

namespace webidl {

void install_core_module(WebidlCoreServices& services) {
  webidl::generated::Webidl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace webidl
