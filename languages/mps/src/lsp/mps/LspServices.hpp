// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "mps/CoreServices.hpp"

namespace mps {

struct MpsSharedServices : MpsSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MpsServices final
    : cursiv::LspServicesFor<MpsCoreServices, MpsSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace mps
