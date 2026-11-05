// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "swiftfin/CoreServices.hpp"

namespace swiftfin {

struct SwiftFinSharedServices : SwiftFinSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SwiftFinServices final
    : cursiv::LspServicesFor<SwiftFinCoreServices, SwiftFinSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace swiftfin
