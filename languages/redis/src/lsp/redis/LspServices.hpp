// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "redis/CoreServices.hpp"

namespace redis {

struct RedisSharedServices : RedisSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct RedisServices final
    : cursiv::LspServicesFor<RedisCoreServices, RedisSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace redis
