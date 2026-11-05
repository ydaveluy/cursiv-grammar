// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "spl/CoreServices.hpp"

namespace spl {

struct SplSharedServices : SplSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SplServices final
    : cursiv::LspServicesFor<SplCoreServices, SplSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace spl
