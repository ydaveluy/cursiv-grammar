// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "evmbytecode/CoreServices.hpp"

namespace evmbytecode {

struct EvmBytecodeSharedServices : EvmBytecodeSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct EvmBytecodeServices final
    : cursiv::LspServicesFor<EvmBytecodeCoreServices, EvmBytecodeSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace evmbytecode
