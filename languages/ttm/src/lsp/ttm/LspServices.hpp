// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "ttm/CoreServices.hpp"

namespace ttm {

struct TtmSharedServices : TtmSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TtmServices final
    : cursiv::LspServicesFor<TtmCoreServices, TtmSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace ttm
