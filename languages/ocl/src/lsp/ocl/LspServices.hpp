// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "ocl/CoreServices.hpp"

namespace ocl {

struct OclSharedServices : OclSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct OclServices final
    : cursiv::LspServicesFor<OclCoreServices, OclSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace ocl
