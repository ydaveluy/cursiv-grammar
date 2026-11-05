// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "gff3/CoreServices.hpp"

namespace gff3 {

struct Gff3SharedServices : Gff3SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Gff3Services final
    : cursiv::LspServicesFor<Gff3CoreServices, Gff3SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace gff3
