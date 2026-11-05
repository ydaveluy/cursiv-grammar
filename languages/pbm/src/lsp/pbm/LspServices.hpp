// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "pbm/CoreServices.hpp"

namespace pbm {

struct PbmSharedServices : PbmSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PbmServices final
    : cursiv::LspServicesFor<PbmCoreServices, PbmSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace pbm
