// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "http/CoreServices.hpp"

namespace http {

struct HttpSharedServices : HttpSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct HttpServices final
    : cursiv::LspServicesFor<HttpCoreServices, HttpSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace http
