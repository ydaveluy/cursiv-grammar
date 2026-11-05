// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "aterm/CoreServices.hpp"

namespace aterm {

struct AtermSharedServices : AtermSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AtermServices final
    : cursiv::LspServicesFor<AtermCoreServices, AtermSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace aterm
