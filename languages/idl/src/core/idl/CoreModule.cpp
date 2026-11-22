// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "idl/CoreServices.hpp"
#include "idl/validation/IdlValidator.hpp"
#include "idl/generated/Idl/Module.hpp"

namespace idl {

void install_core_module(IdlCoreServices& services) {
  idl::generated::Idl::installGeneratedModule(services);
  register_validation_checks(services);
}

}  // namespace idl
