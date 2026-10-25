// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "erlang/core/CoreServices.hpp"
#include "erlang/core/validation/ErlangValidator.hpp"
#include "erlang/generated/Erlang/Module.hpp"

namespace erlang {

void install_core_module(ErlangCoreServices& services) {
  erlang::generated::Erlang::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace erlang
