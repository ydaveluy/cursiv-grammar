// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "cto/CoreServices.hpp"

namespace cto {

struct CtoSharedServices : CtoSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CtoServices final
    : cursiv::LspServicesFor<CtoCoreServices, CtoSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace cto
