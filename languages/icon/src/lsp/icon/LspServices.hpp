// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "icon/CoreServices.hpp"

namespace icon {

struct IconSharedServices : IconSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct IconServices final
    : cursiv::LspServicesFor<IconCoreServices, IconSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace icon
