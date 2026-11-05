// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "yini/CoreServices.hpp"

namespace yini {

struct YiniSharedServices : YiniSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct YiniServices final
    : cursiv::LspServicesFor<YiniCoreServices, YiniSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace yini
