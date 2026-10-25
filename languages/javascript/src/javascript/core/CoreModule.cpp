// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "javascript/core/CoreServices.hpp"
#include "javascript/core/validation/JavaScriptValidator.hpp"
#include "javascript/generated/JavaScript/Module.hpp"

namespace javascript {

void install_core_module(JavaScriptCoreServices& services) {
  javascript::generated::JavaScript::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace javascript
