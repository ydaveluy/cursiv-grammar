// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "metric/CoreServices.hpp"

namespace metric {

struct MetricSharedServices : MetricSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MetricServices final
    : cursiv::LspServicesFor<MetricCoreServices, MetricSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace metric
