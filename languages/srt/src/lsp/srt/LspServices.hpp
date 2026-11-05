// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "srt/CoreServices.hpp"

namespace srt {

struct SrtSharedServices : SrtSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SrtServices final
    : cursiv::LspServicesFor<SrtCoreServices, SrtSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace srt
