// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "tsv/CoreServices.hpp"

namespace tsv {

struct TsvSharedServices : TsvSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TsvServices final
    : cursiv::LspServicesFor<TsvCoreServices, TsvSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace tsv
