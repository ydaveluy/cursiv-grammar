// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "datalog/CoreServices.hpp"

namespace datalog {

struct DatalogSharedServices : DatalogSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct DatalogServices final
    : cursiv::LspServicesFor<DatalogCoreServices, DatalogSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace datalog
