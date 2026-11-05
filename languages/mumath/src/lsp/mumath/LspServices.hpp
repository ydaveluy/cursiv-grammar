// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "mumath/CoreServices.hpp"

namespace mumath {

struct MumathSharedServices : MumathSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MumathServices final
    : cursiv::LspServicesFor<MumathCoreServices, MumathSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace mumath
