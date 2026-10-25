// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "angelscript/core/CoreServices.hpp"
#include "angelscript/core/validation/AngelscriptValidator.hpp"
#include "angelscript/generated/Angelscript/Module.hpp"

namespace angelscript {

void install_core_module(AngelscriptCoreServices& services) {
  angelscript::generated::Angelscript::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace angelscript
