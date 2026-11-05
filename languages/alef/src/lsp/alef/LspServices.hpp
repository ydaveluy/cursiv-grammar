// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "alef/CoreServices.hpp"

namespace alef {

struct AlefSharedServices : AlefSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AlefServices final
    : cursiv::LspServicesFor<AlefCoreServices, AlefSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace alef
