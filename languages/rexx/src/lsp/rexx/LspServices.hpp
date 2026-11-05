// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "rexx/CoreServices.hpp"

namespace rexx {

struct RexxSharedServices : RexxSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct RexxServices final
    : cursiv::LspServicesFor<RexxCoreServices, RexxSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace rexx
