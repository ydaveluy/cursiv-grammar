// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "quakemap/CoreServices.hpp"

namespace quakemap {

struct QuakemapSharedServices : QuakemapSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct QuakemapServices final
    : cursiv::LspServicesFor<QuakemapCoreServices, QuakemapSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace quakemap
