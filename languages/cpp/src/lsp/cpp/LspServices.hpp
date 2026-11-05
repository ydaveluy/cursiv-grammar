// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "cpp/CoreServices.hpp"

namespace cpp {

struct CppSharedServices : CppSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CppServices final
    : cursiv::LspServicesFor<CppCoreServices, CppSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace cpp
