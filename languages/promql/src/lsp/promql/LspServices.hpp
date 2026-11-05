// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "promql/CoreServices.hpp"

namespace promql {

struct PromqlSharedServices : PromqlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PromqlServices final
    : cursiv::LspServicesFor<PromqlCoreServices, PromqlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace promql
