// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "sieve/CoreServices.hpp"

namespace sieve {

struct SieveSharedServices : SieveSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SieveServices final
    : cursiv::LspServicesFor<SieveCoreServices, SieveSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace sieve
