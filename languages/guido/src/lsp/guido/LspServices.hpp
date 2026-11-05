// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "guido/CoreServices.hpp"

namespace guido {

struct GuidoSharedServices : GuidoSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct GuidoServices final
    : cursiv::LspServicesFor<GuidoCoreServices, GuidoSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace guido
