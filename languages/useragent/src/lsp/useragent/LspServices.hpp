// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "useragent/CoreServices.hpp"

namespace useragent {

struct UseragentSharedServices : UseragentSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct UseragentServices final
    : cursiv::LspServicesFor<UseragentCoreServices, UseragentSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace useragent
