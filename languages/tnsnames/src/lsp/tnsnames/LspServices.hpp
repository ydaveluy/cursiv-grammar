// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "tnsnames/CoreServices.hpp"

namespace tnsnames {

struct TnsnamesSharedServices : TnsnamesSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TnsnamesServices final
    : cursiv::LspServicesFor<TnsnamesCoreServices, TnsnamesSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace tnsnames
