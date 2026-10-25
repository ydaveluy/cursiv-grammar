// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "codeql/core/CoreServices.hpp"
#include "codeql/core/validation/CodeqlValidator.hpp"
#include "codeql/generated/Codeql/Module.hpp"

namespace codeql {

void install_core_module(CodeqlCoreServices& services) {
  codeql::generated::Codeql::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace codeql
