// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "racketbsl/CoreServices.hpp"
#include "racketbsl/validation/RacketBslValidator.hpp"
#include "racketbsl/generated/RacketBsl/Module.hpp"

namespace racketbsl {

void install_core_module(RacketBslCoreServices& services) {
  racketbsl::generated::RacketBsl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace racketbsl
