#pragma once
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "redis/ast.hpp"
namespace redis {
class RedisValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkArgument(const ast::Argument &, cursiv::ValidationAcceptor &) const;
  void checkCommand(const ast::Command &, cursiv::ValidationAcceptor &) const;
};
struct RedisCoreServices;
void register_validation_checks(RedisCoreServices &services);
} // namespace redis
