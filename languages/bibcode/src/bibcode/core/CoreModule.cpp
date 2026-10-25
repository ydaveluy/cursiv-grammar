// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "bibcode/core/CoreServices.hpp"
#include "bibcode/core/validation/BibcodeValidator.hpp"
#include "bibcode/generated/Bibcode/Module.hpp"

namespace bibcode {

void install_core_module(BibcodeCoreServices& services) {
  bibcode::generated::Bibcode::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace bibcode
