// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "python/CoreServices.hpp"

namespace python {

struct PythonSharedServices : PythonSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PythonServices final
    : cursiv::LspServicesFor<PythonCoreServices, PythonSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace python
