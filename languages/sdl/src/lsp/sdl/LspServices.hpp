// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "sdl/CoreServices.hpp"

namespace sdl {

struct SdlSharedServices : SdlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SdlServices final
    : cursiv::LspServicesFor<SdlCoreServices, SdlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace sdl
