// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "bicep/CoreServices.hpp"

namespace bicep {

struct BicepSharedServices : BicepSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct BicepServices final
    : cursiv::LspServicesFor<BicepCoreServices, BicepSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace bicep
