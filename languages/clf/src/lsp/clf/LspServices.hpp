// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "clf/CoreServices.hpp"

namespace clf {

struct ClfSharedServices : ClfSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ClfServices final
    : cursiv::LspServicesFor<ClfCoreServices, ClfSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace clf
