// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "calculator/CoreServices.hpp"

namespace calculator {

struct CalculatorSharedServices : CalculatorSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CalculatorServices final
    : cursiv::LspServicesFor<CalculatorCoreServices, CalculatorSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace calculator
