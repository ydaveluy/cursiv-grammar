// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "plantuml/CoreServices.hpp"

namespace plantuml {

struct PlantUMLSharedServices : PlantUMLSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PlantUMLServices final
    : cursiv::LspServicesFor<PlantUMLCoreServices, PlantUMLSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace plantuml
