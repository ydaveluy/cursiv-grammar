// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "gql/CoreServices.hpp"

namespace gql {

struct GqlSharedServices : GqlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct GqlServices final
    : cursiv::LspServicesFor<GqlCoreServices, GqlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace gql
