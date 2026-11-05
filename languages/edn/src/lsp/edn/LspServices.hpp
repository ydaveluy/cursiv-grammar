// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "edn/CoreServices.hpp"

namespace edn {

struct EdnSharedServices : EdnSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct EdnServices final
    : cursiv::LspServicesFor<EdnCoreServices, EdnSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace edn
