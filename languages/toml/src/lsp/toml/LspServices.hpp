// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "toml/CoreServices.hpp"

namespace toml {

struct TomlSharedServices : TomlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TomlServices final
    : cursiv::LspServicesFor<TomlCoreServices, TomlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace toml
