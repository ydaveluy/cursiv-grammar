// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "bcpl/CoreServices.hpp"

namespace bcpl {

struct BcplSharedServices : BcplSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct BcplServices final
    : cursiv::LspServicesFor<BcplCoreServices, BcplSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace bcpl
