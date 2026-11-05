// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "sysmlv2/CoreServices.hpp"

namespace sysmlv2 {

struct SysmlV2SharedServices : SysmlV2SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SysmlV2Services final
    : cursiv::LspServicesFor<SysmlV2CoreServices, SysmlV2SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace sysmlv2
