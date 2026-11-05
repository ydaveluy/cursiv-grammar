// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "redcode/CoreServices.hpp"

namespace redcode {

struct RedcodeSharedServices : RedcodeSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct RedcodeServices final
    : cursiv::LspServicesFor<RedcodeCoreServices, RedcodeSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace redcode
