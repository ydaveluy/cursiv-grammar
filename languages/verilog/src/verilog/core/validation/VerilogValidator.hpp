#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "verilog/core/ast.hpp"

namespace verilog {

class VerilogValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkAssignment(const ast::Assign&, cursiv::ValidationAcceptor&) const;
  void checkStatement(const ast::AssignStmt&, cursiv::ValidationAcceptor&) const;
  void checkProcedural(const ast::ProcAssignStmt&, cursiv::ValidationAcceptor&) const;
  void checkCall(const ast::CallExpr&, cursiv::ValidationAcceptor&) const;
  void checkFunction(const ast::FunctionDecl&, cursiv::ValidationAcceptor&) const;
  void checkTiming(const ast::SystemTimingCheck&, cursiv::ValidationAcceptor&) const;
  void checkInitial(const ast::InitialConstruct&, cursiv::ValidationAcceptor&) const;
  void checkAttribute(const ast::AttributeInstance&, cursiv::ValidationAcceptor&) const;
};

struct VerilogCoreServices;
void register_validation_checks(VerilogCoreServices& services);

}  // namespace verilog
