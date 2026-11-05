// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "pcre/CoreServices.hpp"

namespace pcre {

struct PcreSharedServices : PcreSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PcreServices final
    : cursiv::LspServicesFor<PcreCoreServices, PcreSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace pcre
