// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "basic/CoreServices.hpp"

namespace basic {

struct BasicSharedServices : BasicSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct BasicServices final
    : cursiv::LspServicesFor<BasicCoreServices, BasicSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace basic
