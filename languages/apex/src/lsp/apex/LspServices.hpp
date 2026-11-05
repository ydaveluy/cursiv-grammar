// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "apex/CoreServices.hpp"

namespace apex {

struct ApexSharedServices : ApexSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ApexServices final
    : cursiv::LspServicesFor<ApexCoreServices, ApexSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace apex
