// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "thrift/core/CoreServices.hpp"
#include "thrift/core/validation/ThriftValidator.hpp"
#include "thrift/generated/Thrift/Module.hpp"

namespace thrift {

void install_core_module(ThriftCoreServices& services) {
  thrift::generated::Thrift::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace thrift
