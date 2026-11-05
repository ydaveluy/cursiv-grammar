// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "kquery/CoreServices.hpp"

namespace kquery {

struct KquerySharedServices : KquerySharedCoreServices,
                                           cursiv::SharedLspServices {};

struct KqueryServices final
    : cursiv::LspServicesFor<KqueryCoreServices, KquerySharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace kquery
