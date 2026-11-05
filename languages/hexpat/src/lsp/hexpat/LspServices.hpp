// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "hexpat/CoreServices.hpp"

namespace hexpat {

struct HexpatSharedServices : HexpatSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct HexpatServices final
    : cursiv::LspServicesFor<HexpatCoreServices, HexpatSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace hexpat
