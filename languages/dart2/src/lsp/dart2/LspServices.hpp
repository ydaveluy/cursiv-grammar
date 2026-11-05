// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "dart2/CoreServices.hpp"

namespace dart2 {

struct Dart2SharedServices : Dart2SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Dart2Services final
    : cursiv::LspServicesFor<Dart2CoreServices, Dart2SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace dart2
