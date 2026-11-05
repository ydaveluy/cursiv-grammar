// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "gedcom/CoreServices.hpp"

namespace gedcom {

struct GedcomSharedServices : GedcomSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct GedcomServices final
    : cursiv::LspServicesFor<GedcomCoreServices, GedcomSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace gedcom
