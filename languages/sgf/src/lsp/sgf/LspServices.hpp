// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "sgf/CoreServices.hpp"

namespace sgf {

struct SgfSharedServices : SgfSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SgfServices final
    : cursiv::LspServicesFor<SgfCoreServices, SgfSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace sgf
