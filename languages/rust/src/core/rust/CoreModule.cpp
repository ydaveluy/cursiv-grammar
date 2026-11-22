// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "rust/CoreServices.hpp"
#include "rust/validation/RustValidator.hpp"
#include "rust/generated/Rust/Module.hpp"

namespace rust {

void install_core_module(RustCoreServices& services) {
  rust::generated::Rust::installGeneratedModule(services);
  register_validation_checks(services);
}

}  // namespace rust
