// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "apt/CoreServices.hpp"

namespace apt {

struct AptSharedServices : AptSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AptServices final
    : cursiv::LspServicesFor<AptCoreServices, AptSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace apt
