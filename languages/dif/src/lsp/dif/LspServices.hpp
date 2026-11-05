// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "dif/CoreServices.hpp"

namespace dif {

struct DifSharedServices : DifSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct DifServices final
    : cursiv::LspServicesFor<DifCoreServices, DifSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace dif
