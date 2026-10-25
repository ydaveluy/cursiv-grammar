// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "rust/core/CoreServices.hpp"
#include "rust/core/validation/RustValidator.hpp"
#include "rust/generated/Rust/Module.hpp"

namespace rust {

void install_core_module(RustCoreServices& services) {
  rust::generated::Rust::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace rust
