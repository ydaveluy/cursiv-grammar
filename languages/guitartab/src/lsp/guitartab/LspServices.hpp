// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "guitartab/CoreServices.hpp"

namespace guitartab {

struct GuitartabSharedServices : GuitartabSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct GuitartabServices final
    : cursiv::LspServicesFor<GuitartabCoreServices, GuitartabSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace guitartab
