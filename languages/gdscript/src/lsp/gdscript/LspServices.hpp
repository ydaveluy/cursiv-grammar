// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "gdscript/CoreServices.hpp"

namespace gdscript {

struct GdscriptSharedServices : GdscriptSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct GdscriptServices final
    : cursiv::LspServicesFor<GdscriptCoreServices, GdscriptSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace gdscript
