// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "powerbuilderdw/CoreServices.hpp"

namespace powerbuilderdw {

struct PowerbuilderdwSharedServices : PowerbuilderdwSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PowerbuilderdwServices final
    : cursiv::LspServicesFor<PowerbuilderdwCoreServices, PowerbuilderdwSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace powerbuilderdw
