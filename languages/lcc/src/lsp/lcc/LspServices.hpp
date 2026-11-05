// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "lcc/CoreServices.hpp"

namespace lcc {

struct LccSharedServices : LccSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct LccServices final
    : cursiv::LspServicesFor<LccCoreServices, LccSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace lcc
