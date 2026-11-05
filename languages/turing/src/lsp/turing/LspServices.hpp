// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "turing/CoreServices.hpp"

namespace turing {

struct TuringSharedServices : TuringSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TuringServices final
    : cursiv::LspServicesFor<TuringCoreServices, TuringSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace turing
