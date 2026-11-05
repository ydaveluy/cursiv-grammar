// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "csv/CoreServices.hpp"

namespace csv {

struct CsvSharedServices : CsvSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CsvServices final
    : cursiv::LspServicesFor<CsvCoreServices, CsvSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace csv
