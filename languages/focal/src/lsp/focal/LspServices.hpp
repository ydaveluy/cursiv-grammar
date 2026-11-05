// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "focal/CoreServices.hpp"

namespace focal {

struct FocalSharedServices : FocalSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct FocalServices final
    : cursiv::LspServicesFor<FocalCoreServices, FocalSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace focal
