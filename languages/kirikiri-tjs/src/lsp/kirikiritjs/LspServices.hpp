// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "kirikiritjs/CoreServices.hpp"

namespace kirikiritjs {

struct KirikiriTjsSharedServices : KirikiriTjsSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct KirikiriTjsServices final
    : cursiv::LspServicesFor<KirikiriTjsCoreServices, KirikiriTjsSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace kirikiritjs
