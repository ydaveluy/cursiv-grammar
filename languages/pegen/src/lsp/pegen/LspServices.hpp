// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "pegen/CoreServices.hpp"

namespace pegen {

struct PegenSharedServices : PegenSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PegenServices final
    : cursiv::LspServicesFor<PegenCoreServices, PegenSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace pegen
