// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "typescript/CoreServices.hpp"

namespace typescript {

struct TypeScriptSharedServices : TypeScriptSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TypeScriptServices final
    : cursiv::LspServicesFor<TypeScriptCoreServices, TypeScriptSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace typescript
