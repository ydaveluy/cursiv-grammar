// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "zig/CoreServices.hpp"

namespace zig {

struct ZigSharedServices : ZigSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ZigServices final
    : cursiv::LspServicesFor<ZigCoreServices, ZigSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace zig
