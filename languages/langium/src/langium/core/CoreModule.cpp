// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "langium/core/CoreServices.hpp"
#include "langium/core/validation/LangiumValidator.hpp"
#include "langium/generated/Langium/Module.hpp"

namespace langium {

void install_core_module(LangiumCoreServices& services) {
  langium::generated::Langium::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace langium
