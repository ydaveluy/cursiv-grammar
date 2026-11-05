// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "spass/CoreServices.hpp"

namespace spass {

struct SpassSharedServices : SpassSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SpassServices final
    : cursiv::LspServicesFor<SpassCoreServices, SpassSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace spass
