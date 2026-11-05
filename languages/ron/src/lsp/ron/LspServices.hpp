// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "ron/CoreServices.hpp"

namespace ron {

struct RonSharedServices : RonSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct RonServices final
    : cursiv::LspServicesFor<RonCoreServices, RonSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace ron
