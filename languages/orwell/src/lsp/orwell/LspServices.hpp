// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "orwell/CoreServices.hpp"

namespace orwell {

struct OrwellSharedServices : OrwellSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct OrwellServices final
    : cursiv::LspServicesFor<OrwellCoreServices, OrwellSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace orwell
