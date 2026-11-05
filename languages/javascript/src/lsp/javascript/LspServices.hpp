// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "javascript/CoreServices.hpp"

namespace javascript {

struct JavaScriptSharedServices : JavaScriptSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct JavaScriptServices final
    : cursiv::LspServicesFor<JavaScriptCoreServices, JavaScriptSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace javascript
