// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "tl/CoreServices.hpp"

namespace tl {

struct TlSharedServices : TlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TlServices final
    : cursiv::LspServicesFor<TlCoreServices, TlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace tl
