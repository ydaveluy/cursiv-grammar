// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "protobuf/core/CoreServices.hpp"
#include "protobuf/core/validation/ProtobufValidator.hpp"
#include "protobuf/generated/Protobuf/Module.hpp"

namespace protobuf {

void install_core_module(ProtobufCoreServices& services) {
  protobuf::generated::Protobuf::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace protobuf
