// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "pddl/CoreServices.hpp"

namespace pddl {

struct PddlSharedServices : PddlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PddlServices final
    : cursiv::LspServicesFor<PddlCoreServices, PddlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace pddl
