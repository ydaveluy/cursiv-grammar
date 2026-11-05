// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "antlr/CoreServices.hpp"

namespace antlr {

struct AntlrSharedServices : AntlrSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AntlrServices final
    : cursiv::LspServicesFor<AntlrCoreServices, AntlrSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace antlr
