// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "typescript/core/CoreServices.hpp"
#include "typescript/core/validation/TypeScriptValidator.hpp"
#include "typescript/generated/TypeScript/Module.hpp"

namespace typescript {

void install_core_module(TypeScriptCoreServices& services) {
  typescript::generated::TypeScript::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace typescript
