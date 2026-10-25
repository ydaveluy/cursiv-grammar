// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "capnproto/core/CoreServices.hpp"
#include "capnproto/core/validation/CapnprotoValidator.hpp"
#include "capnproto/generated/Capnproto/Module.hpp"

namespace capnproto {

void install_core_module(CapnprotoCoreServices& services) {
  capnproto::generated::Capnproto::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace capnproto
