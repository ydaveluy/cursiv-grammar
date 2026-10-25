// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "redis/core/CoreServices.hpp"
#include "redis/core/validation/RedisValidator.hpp"
#include "redis/generated/Redis/Module.hpp"

namespace redis {

void install_core_module(RedisCoreServices& services) {
  redis::generated::Redis::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace redis
