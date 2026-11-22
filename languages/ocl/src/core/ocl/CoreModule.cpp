// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "ocl/CoreServices.hpp"
#include "ocl/validation/OclValidator.hpp"
#include "ocl/generated/Ocl/Module.hpp"

namespace ocl {

void install_core_module(OclCoreServices& services) {
  ocl::generated::Ocl::installGeneratedModule(services);
  register_validation_checks(services);
}

}  // namespace ocl
