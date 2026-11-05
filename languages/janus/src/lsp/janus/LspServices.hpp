// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "janus/CoreServices.hpp"

namespace janus {

struct JanusSharedServices : JanusSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct JanusServices final
    : cursiv::LspServicesFor<JanusCoreServices, JanusSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace janus
