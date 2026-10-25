#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "wren/core/ast.hpp"

namespace wren {

class WrenValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkMember(const ast::Member&, cursiv::ValidationAcceptor&) const;
  void checkAssignment(const ast::AssignStat&, cursiv::ValidationAcceptor&) const;
};

struct WrenCoreServices;
void register_validation_checks(WrenCoreServices& services);

}  // namespace wren
