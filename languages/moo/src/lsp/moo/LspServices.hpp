// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "moo/CoreServices.hpp"

namespace moo {

struct MooSharedServices : MooSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MooServices final
    : cursiv::LspServicesFor<MooCoreServices, MooSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace moo
