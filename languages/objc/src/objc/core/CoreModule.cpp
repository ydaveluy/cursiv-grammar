// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "objc/core/CoreServices.hpp"
#include "objc/core/validation/ObjcValidator.hpp"
#include "objc/generated/Objc/Module.hpp"

namespace objc {

void install_core_module(ObjcCoreServices& services) {
  objc::generated::Objc::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace objc
