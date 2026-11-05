// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "lark/CoreServices.hpp"

namespace lark {

struct LarkSharedServices : LarkSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct LarkServices final
    : cursiv::LspServicesFor<LarkCoreServices, LarkSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace lark
