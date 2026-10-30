// Local textual constraints only; model/type/library resolution is separate.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "sysmlv2/core/ast.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace sysmlv2 {

class SysmlV2Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkPostfix(const ast::Postfix&, cursiv::ValidationAcceptor&) const;
  void checkSpec(const ast::Spec&, cursiv::ValidationAcceptor&) const;
  void checkElement(const ast::Element&, cursiv::ValidationAcceptor&) const;
  void checkBinary(const ast::Binary&, cursiv::ValidationAcceptor&) const;
};

struct SysmlV2CoreServices;
void register_validation_checks(SysmlV2CoreServices& services);

}  // namespace sysmlv2
