// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "matlab/CoreServices.hpp"

namespace matlab {

struct MatlabSharedServices : MatlabSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MatlabServices final
    : cursiv::LspServicesFor<MatlabCoreServices, MatlabSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace matlab
