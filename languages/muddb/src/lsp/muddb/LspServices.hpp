// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "muddb/CoreServices.hpp"

namespace muddb {

struct MuddbSharedServices : MuddbSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MuddbServices final
    : cursiv::LspServicesFor<MuddbCoreServices, MuddbSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace muddb
