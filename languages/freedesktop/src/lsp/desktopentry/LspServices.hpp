// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "desktopentry/CoreServices.hpp"

namespace desktopentry {

struct DesktopEntrySharedServices : DesktopEntrySharedCoreServices,
                                           cursiv::SharedLspServices {};

struct DesktopEntryServices final
    : cursiv::LspServicesFor<DesktopEntryCoreServices, DesktopEntrySharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace desktopentry
