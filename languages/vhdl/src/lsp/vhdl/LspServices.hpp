// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "vhdl/CoreServices.hpp"

namespace vhdl {

struct VhdlSharedServices : VhdlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct VhdlServices final
    : cursiv::LspServicesFor<VhdlCoreServices, VhdlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace vhdl
