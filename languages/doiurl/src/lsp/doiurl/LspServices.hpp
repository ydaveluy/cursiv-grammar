// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "doiurl/CoreServices.hpp"

namespace doiurl {

struct DoiurlSharedServices : DoiurlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct DoiurlServices final
    : cursiv::LspServicesFor<DoiurlCoreServices, DoiurlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace doiurl
