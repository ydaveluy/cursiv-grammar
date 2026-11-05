// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "yara/CoreServices.hpp"

namespace yara {

struct YaraSharedServices : YaraSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct YaraServices final
    : cursiv::LspServicesFor<YaraCoreServices, YaraSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace yara
