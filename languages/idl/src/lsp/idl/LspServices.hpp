// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "idl/CoreServices.hpp"

namespace idl {

struct IdlSharedServices : IdlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct IdlServices final
    : cursiv::LspServicesFor<IdlCoreServices, IdlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace idl
