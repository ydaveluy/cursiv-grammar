// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "microc/CoreServices.hpp"

namespace microc {

struct MicrocSharedServices : MicrocSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MicrocServices final
    : cursiv::LspServicesFor<MicrocCoreServices, MicrocSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace microc
