// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "css3/CoreServices.hpp"

namespace css3 {

struct Css3SharedServices : Css3SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Css3Services final
    : cursiv::LspServicesFor<Css3CoreServices, Css3SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace css3
