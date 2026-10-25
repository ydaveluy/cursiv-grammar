// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "elixir/core/CoreServices.hpp"
#include "elixir/core/validation/ElixirValidator.hpp"
#include "elixir/generated/Elixir/Module.hpp"

namespace elixir {

void install_core_module(ElixirCoreServices& services) {
  elixir::generated::Elixir::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace elixir
