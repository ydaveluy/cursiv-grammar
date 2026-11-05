// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "gtin/CoreServices.hpp"

namespace gtin {

struct GtinSharedServices : GtinSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct GtinServices final
    : cursiv::LspServicesFor<GtinCoreServices, GtinSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace gtin
