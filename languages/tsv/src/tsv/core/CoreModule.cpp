// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "tsv/core/CoreServices.hpp"
#include "tsv/core/validation/TsvValidator.hpp"
#include "tsv/generated/Tsv/Module.hpp"

namespace tsv {

void install_core_module(TsvCoreServices& services) {
  tsv::generated::Tsv::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace tsv
