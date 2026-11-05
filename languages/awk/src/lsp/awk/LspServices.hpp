// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "awk/CoreServices.hpp"

namespace awk {

struct AwkSharedServices : AwkSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AwkServices final
    : cursiv::LspServicesFor<AwkCoreServices, AwkSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace awk
