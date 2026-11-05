// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "tiny/CoreServices.hpp"

namespace tiny {

struct TinySharedServices : TinySharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TinyServices final
    : cursiv::LspServicesFor<TinyCoreServices, TinySharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace tiny
