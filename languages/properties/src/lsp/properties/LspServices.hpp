// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "properties/CoreServices.hpp"

namespace properties {

struct PropertiesSharedServices : PropertiesSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PropertiesServices final
    : cursiv::LspServicesFor<PropertiesCoreServices, PropertiesSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace properties
