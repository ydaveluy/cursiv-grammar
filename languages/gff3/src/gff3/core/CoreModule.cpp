// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "gff3/core/CoreServices.hpp"
#include "gff3/core/validation/Gff3Validator.hpp"
#include "gff3/generated/Gff3/Module.hpp"

namespace gff3 {

void install_core_module(Gff3CoreServices& services) {
  gff3::generated::Gff3::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace gff3
