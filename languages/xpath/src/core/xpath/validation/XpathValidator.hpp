#pragma once
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "xpath/ast.hpp"
namespace xpath {
class XpathValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkFunction(const ast::InlineFunctionExpr &,
                     cursiv::ValidationAcceptor &) const;
  void checkCall(const ast::CallExpr &, cursiv::ValidationAcceptor &) const;
  void checkReference(const ast::NamedFunctionRef &,
                      cursiv::ValidationAcceptor &) const;
};
struct XpathCoreServices;
void register_validation_checks(XpathCoreServices &services);
} // namespace xpath
