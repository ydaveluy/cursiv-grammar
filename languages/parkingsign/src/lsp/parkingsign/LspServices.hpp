// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "parkingsign/CoreServices.hpp"

namespace parkingsign {

struct ParkingsignSharedServices : ParkingsignSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ParkingsignServices final
    : cursiv::LspServicesFor<ParkingsignCoreServices, ParkingsignSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace parkingsign
