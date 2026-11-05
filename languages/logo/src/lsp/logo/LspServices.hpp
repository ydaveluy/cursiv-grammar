// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "logo/CoreServices.hpp"

namespace logo {

struct LogoSharedServices : LogoSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct LogoServices final
    : cursiv::LspServicesFor<LogoCoreServices, LogoSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace logo
