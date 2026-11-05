// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "ltl/CoreServices.hpp"

namespace ltl {

struct LtlSharedServices : LtlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct LtlServices final
    : cursiv::LspServicesFor<LtlCoreServices, LtlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace ltl
