// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "cobol85/CoreServices.hpp"

namespace cobol85 {

struct Cobol85SharedServices : Cobol85SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Cobol85Services final
    : cursiv::LspServicesFor<Cobol85CoreServices, Cobol85SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace cobol85
