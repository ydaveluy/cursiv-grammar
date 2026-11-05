// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "rust/CoreServices.hpp"

namespace rust {

struct RustSharedServices : RustSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct RustServices final
    : cursiv::LspServicesFor<RustCoreServices, RustSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace rust
