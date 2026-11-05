// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "vaxscan/CoreServices.hpp"

namespace vaxscan {

struct VaxscanSharedServices : VaxscanSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct VaxscanServices final
    : cursiv::LspServicesFor<VaxscanCoreServices, VaxscanSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace vaxscan
