#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "xsmp/core/ast.hpp"

namespace xsmp {
class XsmpValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkName(const ast::NamedElement&, cursiv::ValidationAcceptor&) const;
  void checkConstant(const ast::Constant&, cursiv::ValidationAcceptor&) const;
  void checkAttribute(const ast::Attribute&, cursiv::ValidationAcceptor&) const;
};
struct XsmpCoreServices;
void register_validation_checks(XsmpCoreServices& services);
}  // namespace xsmp
