// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "trapc/CoreServices.hpp"

namespace trapc {

struct TrapcSharedServices : TrapcSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TrapcServices final
    : cursiv::LspServicesFor<TrapcCoreServices, TrapcSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace trapc
