// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "antlr/core/CoreServices.hpp"
#include "antlr/core/validation/AntlrValidator.hpp"
#include "antlr/generated/Antlr/Module.hpp"

namespace antlr {

void install_core_module(AntlrCoreServices& services) {
  antlr::generated::Antlr::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace antlr
