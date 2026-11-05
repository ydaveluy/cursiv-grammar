// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "ada/CoreServices.hpp"

namespace ada {

struct AdaSharedServices : AdaSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AdaServices final
    : cursiv::LspServicesFor<AdaCoreServices, AdaSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace ada
