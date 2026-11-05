// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "metamath/CoreServices.hpp"

namespace metamath {

struct MetamathSharedServices : MetamathSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MetamathServices final
    : cursiv::LspServicesFor<MetamathCoreServices, MetamathSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace metamath
