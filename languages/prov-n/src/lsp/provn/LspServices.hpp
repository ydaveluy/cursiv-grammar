// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "provn/CoreServices.hpp"

namespace provn {

struct ProvNSharedServices : ProvNSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ProvNServices final
    : cursiv::LspServicesFor<ProvNCoreServices, ProvNSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace provn
