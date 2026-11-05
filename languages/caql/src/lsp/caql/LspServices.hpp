// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "caql/CoreServices.hpp"

namespace caql {

struct CaqlSharedServices : CaqlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CaqlServices final
    : cursiv::LspServicesFor<CaqlCoreServices, CaqlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace caql
