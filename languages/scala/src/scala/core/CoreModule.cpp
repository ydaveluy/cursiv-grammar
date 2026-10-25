// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "scala/core/CoreServices.hpp"
#include "scala/core/validation/ScalaValidator.hpp"
#include "scala/generated/Scala/Module.hpp"

namespace scala {

void install_core_module(ScalaCoreServices& services) {
  scala::generated::Scala::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace scala
