// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "ply/CoreServices.hpp"

namespace ply {

struct PlySharedServices : PlySharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PlyServices final
    : cursiv::LspServicesFor<PlyCoreServices, PlySharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace ply
