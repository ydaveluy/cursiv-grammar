// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "mckeemanform/core/CoreServices.hpp"
#include "mckeemanform/core/validation/MckeemanFormValidator.hpp"
#include "mckeemanform/generated/MckeemanForm/Module.hpp"

namespace mckeemanform {

void install_core_module(MckeemanFormCoreServices& services) {
  mckeemanform::generated::MckeemanForm::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace mckeemanform
