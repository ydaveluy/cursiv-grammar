// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "less/CoreServices.hpp"

namespace less {

struct LessSharedServices : LessSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct LessServices final
    : cursiv::LspServicesFor<LessCoreServices, LessSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace less
