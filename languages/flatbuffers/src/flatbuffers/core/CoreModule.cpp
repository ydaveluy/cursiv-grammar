// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "flatbuffers/core/CoreServices.hpp"
#include "flatbuffers/core/validation/FlatbuffersValidator.hpp"
#include "flatbuffers/generated/Flatbuffers/Module.hpp"

namespace flatbuffers {

void install_core_module(FlatbuffersCoreServices& services) {
  flatbuffers::generated::Flatbuffers::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace flatbuffers
