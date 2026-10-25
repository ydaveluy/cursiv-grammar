// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "racketisl/core/CoreServices.hpp"
#include "racketisl/core/validation/RacketIslValidator.hpp"
#include "racketisl/generated/RacketIsl/Module.hpp"

namespace racketisl {

void install_core_module(RacketIslCoreServices& services) {
  racketisl::generated::RacketIsl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace racketisl
