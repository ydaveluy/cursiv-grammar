// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "tcpheader/core/CoreServices.hpp"
#include "tcpheader/core/validation/TcpheaderValidator.hpp"
#include "tcpheader/generated/Tcpheader/Module.hpp"

namespace tcpheader {

void install_core_module(TcpheaderCoreServices& services) {
  tcpheader::generated::Tcpheader::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace tcpheader
