// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "vmf/CoreServices.hpp"

namespace vmf {

struct VmfSharedServices : VmfSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct VmfServices final
    : cursiv::LspServicesFor<VmfCoreServices, VmfSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace vmf
