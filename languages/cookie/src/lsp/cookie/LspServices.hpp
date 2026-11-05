// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "cookie/CoreServices.hpp"

namespace cookie {

struct CookieSharedServices : CookieSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CookieServices final
    : cursiv::LspServicesFor<CookieCoreServices, CookieSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace cookie
