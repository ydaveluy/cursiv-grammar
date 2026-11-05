// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "clu/CoreServices.hpp"

namespace clu {

struct CluSharedServices : CluSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CluServices final
    : cursiv::LspServicesFor<CluCoreServices, CluSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace clu
