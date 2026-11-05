// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "bdf/CoreServices.hpp"

namespace bdf {

struct BdfSharedServices : BdfSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct BdfServices final
    : cursiv::LspServicesFor<BdfCoreServices, BdfSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace bdf
