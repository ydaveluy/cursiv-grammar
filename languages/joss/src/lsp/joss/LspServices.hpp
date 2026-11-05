// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "joss/CoreServices.hpp"

namespace joss {

struct JossSharedServices : JossSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct JossServices final
    : cursiv::LspServicesFor<JossCoreServices, JossSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace joss
