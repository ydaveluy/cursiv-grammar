// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "oncrpc/CoreServices.hpp"

namespace oncrpc {

struct OncrpcSharedServices : OncrpcSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct OncrpcServices final
    : cursiv::LspServicesFor<OncrpcCoreServices, OncrpcSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace oncrpc
