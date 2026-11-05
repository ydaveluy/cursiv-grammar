// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "modelica/CoreServices.hpp"

namespace modelica {

struct ModelicaSharedServices : ModelicaSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ModelicaServices final
    : cursiv::LspServicesFor<ModelicaCoreServices, ModelicaSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace modelica
