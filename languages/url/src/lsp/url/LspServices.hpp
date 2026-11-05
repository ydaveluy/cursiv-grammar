// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "url/CoreServices.hpp"

namespace url {

struct UrlSharedServices : UrlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct UrlServices final
    : cursiv::LspServicesFor<UrlCoreServices, UrlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace url
