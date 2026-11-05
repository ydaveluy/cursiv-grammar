// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "xyz/CoreServices.hpp"

namespace xyz {

struct XyzSharedServices : XyzSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct XyzServices final
    : cursiv::LspServicesFor<XyzCoreServices, XyzSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace xyz
