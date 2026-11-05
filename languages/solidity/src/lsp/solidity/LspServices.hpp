// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "solidity/CoreServices.hpp"

namespace solidity {

struct SoliditySharedServices : SoliditySharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SolidityServices final
    : cursiv::LspServicesFor<SolidityCoreServices, SoliditySharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace solidity
