// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "mumps/CoreServices.hpp"

namespace mumps {

struct MumpsSharedServices : MumpsSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MumpsServices final
    : cursiv::LspServicesFor<MumpsCoreServices, MumpsSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace mumps
