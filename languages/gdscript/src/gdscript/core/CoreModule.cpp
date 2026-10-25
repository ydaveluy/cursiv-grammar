// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "gdscript/core/CoreServices.hpp"
#include "gdscript/core/validation/GdscriptValidator.hpp"
#include "gdscript/generated/Gdscript/Module.hpp"

namespace gdscript {

void install_core_module(GdscriptCoreServices& services) {
  gdscript::generated::Gdscript::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace gdscript
