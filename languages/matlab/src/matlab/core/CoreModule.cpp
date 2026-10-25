// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "matlab/core/CoreServices.hpp"
#include "matlab/core/validation/MatlabValidator.hpp"
#include "matlab/generated/Matlab/Module.hpp"

namespace matlab {

void install_core_module(MatlabCoreServices& services) {
  matlab::generated::Matlab::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace matlab
