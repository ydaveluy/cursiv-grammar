#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "memcachedprotocol/ast.hpp"

namespace memcachedprotocol {

class MemcachedProtocolValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkCommand(const ast::Command& node, cursiv::ValidationAcceptor& accept) const;
};

struct MemcachedProtocolCoreServices;
void register_validation_checks(MemcachedProtocolCoreServices& services);

}  // namespace memcachedprotocol
