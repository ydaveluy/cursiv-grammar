// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "upnp/CoreServices.hpp"

namespace upnp {

struct UpnpSharedServices : UpnpSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct UpnpServices final
    : cursiv::LspServicesFor<UpnpCoreServices, UpnpSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace upnp
