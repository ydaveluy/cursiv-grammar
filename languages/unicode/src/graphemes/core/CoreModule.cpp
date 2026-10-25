// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "graphemes/core/CoreServices.hpp"
#include "graphemes/core/validation/GraphemesValidator.hpp"
#include "graphemes/generated/Graphemes/Module.hpp"

namespace graphemes {

void install_core_module(GraphemesCoreServices& services) {
  graphemes::generated::Graphemes::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace graphemes
