// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "peoplecode/CoreServices.hpp"

namespace peoplecode {

struct PeoplecodeSharedServices : PeoplecodeSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PeoplecodeServices final
    : cursiv::LspServicesFor<PeoplecodeCoreServices, PeoplecodeSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace peoplecode
