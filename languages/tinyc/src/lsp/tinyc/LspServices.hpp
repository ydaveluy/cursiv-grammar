// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "tinyc/CoreServices.hpp"

namespace tinyc {

struct TinycSharedServices : TinycSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TinycServices final
    : cursiv::LspServicesFor<TinycCoreServices, TinycSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace tinyc
