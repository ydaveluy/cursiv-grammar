// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "cayenne/CoreServices.hpp"

namespace cayenne {

struct CayenneSharedServices : CayenneSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CayenneServices final
    : cursiv::LspServicesFor<CayenneCoreServices, CayenneSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace cayenne
