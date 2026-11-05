// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "lpc/CoreServices.hpp"

namespace lpc {

struct LpcSharedServices : LpcSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct LpcServices final
    : cursiv::LspServicesFor<LpcCoreServices, LpcSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace lpc
