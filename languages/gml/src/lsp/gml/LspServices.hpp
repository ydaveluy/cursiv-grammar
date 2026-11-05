// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "gml/CoreServices.hpp"

namespace gml {

struct GmlSharedServices : GmlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct GmlServices final
    : cursiv::LspServicesFor<GmlCoreServices, GmlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace gml
