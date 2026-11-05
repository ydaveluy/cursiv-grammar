// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "newick/CoreServices.hpp"

namespace newick {

struct NewickSharedServices : NewickSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct NewickServices final
    : cursiv::LspServicesFor<NewickCoreServices, NewickSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace newick
