// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "mckeemanform/CoreServices.hpp"

namespace mckeemanform {

struct MckeemanFormSharedServices : MckeemanFormSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MckeemanFormServices final
    : cursiv::LspServicesFor<MckeemanFormCoreServices, MckeemanFormSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace mckeemanform
