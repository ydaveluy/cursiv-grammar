// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "wktcrsv1/CoreServices.hpp"

namespace wktcrsv1 {

struct WktCrsV1SharedServices : WktCrsV1SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct WktCrsV1Services final
    : cursiv::LspServicesFor<WktCrsV1CoreServices, WktCrsV1SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace wktcrsv1
