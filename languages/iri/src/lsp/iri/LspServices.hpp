// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "iri/CoreServices.hpp"

namespace iri {

struct IriSharedServices : IriSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct IriServices final
    : cursiv::LspServicesFor<IriCoreServices, IriSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace iri
