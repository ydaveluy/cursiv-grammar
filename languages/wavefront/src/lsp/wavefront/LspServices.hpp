// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "wavefront/CoreServices.hpp"

namespace wavefront {

struct WavefrontSharedServices : WavefrontSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct WavefrontServices final
    : cursiv::LspServicesFor<WavefrontCoreServices, WavefrontSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace wavefront
