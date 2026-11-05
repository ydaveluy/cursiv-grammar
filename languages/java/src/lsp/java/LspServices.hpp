// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "java/CoreServices.hpp"

namespace java {

struct JavaSharedServices : JavaSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct JavaServices final
    : cursiv::LspServicesFor<JavaCoreServices, JavaSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace java
