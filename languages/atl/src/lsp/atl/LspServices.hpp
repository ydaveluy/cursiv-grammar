// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "atl/CoreServices.hpp"

namespace atl {

struct AtlSharedServices : AtlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AtlServices final
    : cursiv::LspServicesFor<AtlCoreServices, AtlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace atl
