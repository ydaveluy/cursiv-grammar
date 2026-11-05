// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "sparql/CoreServices.hpp"

namespace sparql {

struct SparqlSharedServices : SparqlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SparqlServices final
    : cursiv::LspServicesFor<SparqlCoreServices, SparqlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace sparql
