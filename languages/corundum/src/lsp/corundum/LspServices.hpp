// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "corundum/CoreServices.hpp"

namespace corundum {

struct CorundumSharedServices : CorundumSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CorundumServices final
    : cursiv::LspServicesFor<CorundumCoreServices, CorundumSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace corundum
