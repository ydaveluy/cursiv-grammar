// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "informix/CoreServices.hpp"

namespace informix {

struct InformixSharedServices : InformixSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct InformixServices final
    : cursiv::LspServicesFor<InformixCoreServices, InformixSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace informix
