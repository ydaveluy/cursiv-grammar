// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "rego/CoreServices.hpp"

namespace rego {

struct RegoSharedServices : RegoSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct RegoServices final
    : cursiv::LspServicesFor<RegoCoreServices, RegoSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace rego
