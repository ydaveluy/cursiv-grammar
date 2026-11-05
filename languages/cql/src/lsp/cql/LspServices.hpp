// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "cql/CoreServices.hpp"

namespace cql {

struct CqlSharedServices : CqlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CqlServices final
    : cursiv::LspServicesFor<CqlCoreServices, CqlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace cql
