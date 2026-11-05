// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "webidl/CoreServices.hpp"

namespace webidl {

struct WebidlSharedServices : WebidlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct WebidlServices final
    : cursiv::LspServicesFor<WebidlCoreServices, WebidlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace webidl
