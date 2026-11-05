// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "itn/CoreServices.hpp"

namespace itn {

struct ItnSharedServices : ItnSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ItnServices final
    : cursiv::LspServicesFor<ItnCoreServices, ItnSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace itn
