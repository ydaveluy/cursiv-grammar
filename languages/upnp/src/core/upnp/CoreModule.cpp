// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "upnp/CoreServices.hpp"
#include "upnp/validation/UpnpValidator.hpp"
#include "upnp/generated/Upnp/Module.hpp"

namespace upnp {

void install_core_module(UpnpCoreServices& services) {
  upnp::generated::Upnp::installGeneratedModule(services);
  register_validation_checks(services);
}

}  // namespace upnp
