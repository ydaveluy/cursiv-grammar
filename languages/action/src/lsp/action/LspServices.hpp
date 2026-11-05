// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "action/CoreServices.hpp"

namespace action {

struct ActionSharedServices : ActionSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ActionServices final
    : cursiv::LspServicesFor<ActionCoreServices, ActionSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace action
