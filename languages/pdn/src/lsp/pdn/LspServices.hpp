// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "pdn/CoreServices.hpp"

namespace pdn {

struct PdnSharedServices : PdnSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PdnServices final
    : cursiv::LspServicesFor<PdnCoreServices, PdnSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace pdn
