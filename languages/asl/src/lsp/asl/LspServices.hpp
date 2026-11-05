// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "asl/CoreServices.hpp"

namespace asl {

struct AslSharedServices : AslSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AslServices final
    : cursiv::LspServicesFor<AslCoreServices, AslSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace asl
