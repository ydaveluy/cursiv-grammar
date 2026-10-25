// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "cobol85/core/CoreServices.hpp"
#include "cobol85/core/validation/Cobol85Validator.hpp"
#include "cobol85/generated/Cobol85/Module.hpp"

namespace cobol85 {

void install_core_module(Cobol85CoreServices& services) {
  cobol85::generated::Cobol85::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace cobol85
