// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "arithmetic/CoreServices.hpp"

namespace arithmetic {

struct ArithmeticSharedServices : ArithmeticSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ArithmeticServices final
    : cursiv::LspServicesFor<ArithmeticCoreServices, ArithmeticSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace arithmetic
