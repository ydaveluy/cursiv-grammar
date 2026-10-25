// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "fortran/core/CoreServices.hpp"
#include "fortran/core/validation/FortranValidator.hpp"
#include "fortran/generated/Fortran/Module.hpp"

namespace fortran {

void install_core_module(FortranCoreServices& services) {
  fortran::generated::Fortran::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace fortran
