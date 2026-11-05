// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "bibtex/CoreServices.hpp"

namespace bibtex {

struct BibtexSharedServices : BibtexSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct BibtexServices final
    : cursiv::LspServicesFor<BibtexCoreServices, BibtexSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace bibtex
