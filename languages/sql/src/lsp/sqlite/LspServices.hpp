// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "sqlite/CoreServices.hpp"

namespace sqlite {

struct SqliteSharedServices : SqliteSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SqliteServices final
    : cursiv::LspServicesFor<SqliteCoreServices, SqliteSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace sqlite
