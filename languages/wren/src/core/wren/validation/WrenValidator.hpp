#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "wren/ast.hpp"

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
