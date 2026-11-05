// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "jpa/CoreServices.hpp"

namespace jpa {

struct JpaSharedServices : JpaSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct JpaServices final
    : cursiv::LspServicesFor<JpaCoreServices, JpaSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace jpa
