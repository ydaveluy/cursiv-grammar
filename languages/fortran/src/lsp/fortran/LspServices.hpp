// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "fortran/CoreServices.hpp"

namespace fortran {

struct FortranSharedServices : FortranSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct FortranServices final
    : cursiv::LspServicesFor<FortranCoreServices, FortranSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace fortran
