// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "javacc/CoreServices.hpp"

namespace javacc {

struct JavaccSharedServices : JavaccSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct JavaccServices final
    : cursiv::LspServicesFor<JavaccCoreServices, JavaccSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace javacc
