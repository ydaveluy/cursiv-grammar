// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "llvmir/core/CoreServices.hpp"
#include "llvmir/core/validation/LlvmIrValidator.hpp"
#include "llvmir/generated/LlvmIr/Module.hpp"

namespace llvmir {

void install_core_module(LlvmIrCoreServices& services) {
  llvmir::generated::LlvmIr::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace llvmir
