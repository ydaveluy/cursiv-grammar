// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "v/CoreServices.hpp"

namespace v {

struct VSharedServices : VSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct VServices final
    : cursiv::LspServicesFor<VCoreServices, VSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace v
