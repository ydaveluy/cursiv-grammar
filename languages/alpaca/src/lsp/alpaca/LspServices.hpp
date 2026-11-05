// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "alpaca/CoreServices.hpp"

namespace alpaca {

struct AlpacaSharedServices : AlpacaSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AlpacaServices final
    : cursiv::LspServicesFor<AlpacaCoreServices, AlpacaSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace alpaca
