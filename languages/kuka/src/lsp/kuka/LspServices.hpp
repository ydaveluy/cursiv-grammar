// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "kuka/CoreServices.hpp"

namespace kuka {

struct KukaSharedServices : KukaSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct KukaServices final
    : cursiv::LspServicesFor<KukaCoreServices, KukaSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace kuka
