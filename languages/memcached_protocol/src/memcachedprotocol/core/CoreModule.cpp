// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "memcachedprotocol/core/CoreServices.hpp"
#include "memcachedprotocol/core/validation/MemcachedProtocolValidator.hpp"
#include "memcachedprotocol/generated/MemcachedProtocol/Module.hpp"

namespace memcachedprotocol {

void install_core_module(MemcachedProtocolCoreServices& services) {
  memcachedprotocol::generated::MemcachedProtocol::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace memcachedprotocol
