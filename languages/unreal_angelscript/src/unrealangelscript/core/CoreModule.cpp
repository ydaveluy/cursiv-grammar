// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "unrealangelscript/core/CoreServices.hpp"
#include "unrealangelscript/core/validation/UnrealAngelscriptValidator.hpp"
#include "unrealangelscript/generated/UnrealAngelscript/Module.hpp"

namespace unrealangelscript {

void install_core_module(UnrealAngelscriptCoreServices& services) {
  unrealangelscript::generated::UnrealAngelscript::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace unrealangelscript
