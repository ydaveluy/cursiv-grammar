// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "lambda/CoreServices.hpp"

namespace lambda {

struct LambdaSharedServices : LambdaSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct LambdaServices final
    : cursiv::LspServicesFor<LambdaCoreServices, LambdaSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace lambda
