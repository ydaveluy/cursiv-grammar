// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "nasm/CoreServices.hpp"
#include "nasm/validation/NasmValidator.hpp"
#include "nasm/generated/Nasm/Module.hpp"

namespace nasm {

void install_core_module(NasmCoreServices& services) {
  nasm::generated::Nasm::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace nasm
