#pragma once
#include "blueprint/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
namespace blueprint {
class BlueprintValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkInteger(const ast::IntegerLit &,
                    cursiv::ValidationAcceptor &) const;
  void checkString(const ast::StringLit &, cursiv::ValidationAcceptor &) const;
  void checkProperty(const ast::Property &, cursiv::ValidationAcceptor &) const;
  void checkModule(const ast::Module &, cursiv::ValidationAcceptor &) const;
  void checkList(const ast::ListValue &, cursiv::ValidationAcceptor &) const;
  void checkMap(const ast::MapValue &, cursiv::ValidationAcceptor &) const;
  void checkCondition(const ast::Condition &,
                      cursiv::ValidationAcceptor &) const;
  void checkVariable(const ast::Variable &, cursiv::ValidationAcceptor &) const;
  void checkBinding(const ast::AnyPattern &,
                    cursiv::ValidationAcceptor &) const;
  void checkAssignment(const ast::Assignment &,
                       cursiv::ValidationAcceptor &) const;
  void checkSelect(const ast::Select &, cursiv::ValidationAcceptor &) const;
};
struct BlueprintCoreServices;
void register_validation_checks(BlueprintCoreServices &services);
} // namespace blueprint
