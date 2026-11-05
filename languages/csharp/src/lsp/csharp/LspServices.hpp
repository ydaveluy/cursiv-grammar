// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "csharp/CoreServices.hpp"

namespace csharp {

struct CSharpSharedServices : CSharpSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CSharpServices final
    : cursiv::LspServicesFor<CSharpCoreServices, CSharpSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace csharp
