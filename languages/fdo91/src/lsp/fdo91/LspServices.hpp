// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "fdo91/CoreServices.hpp"

namespace fdo91 {

struct Fdo91SharedServices : Fdo91SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Fdo91Services final
    : cursiv::LspServicesFor<Fdo91CoreServices, Fdo91SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace fdo91
