#include "verilog/core/validation/VerilogValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "verilog/core/CoreServices.hpp"

namespace verilog {
namespace {
bool assignable(const ast::Expr* expression) {
  if (cursiv::ast_cast<ast::HierIdent>(expression) ||
      cursiv::ast_cast<ast::PreprocessorExpr>(expression)) return true;
  if (auto x = cursiv::ast_cast<ast::IndexExpr>(expression))
    return assignable(x->base);
  if (auto x = cursiv::ast_cast<ast::RangeSelectExpr>(expression))
    return assignable(x->base);
  if (auto x = cursiv::ast_cast<ast::PartSelectExpr>(expression))
    return assignable(x->base);
  if (auto x = cursiv::ast_cast<ast::ConcatExpr>(expression)) {
    for (auto item : x->items)
      if (!assignable(item)) return false;
    return !x->items.empty();
  }
  return false;
}

template<class T>
void checkTarget(const T& node, cursiv::ValidationAcceptor& accept) {
  if (node.lvalue && !assignable(node.lvalue))
    accept.error(node, "An assignment target must be a variable, select or concatenation",
                 "verilog.assignment-target").template property<&T::lvalue>();
}
}  // namespace

void VerilogValidator::checkAssignment(const ast::Assign& node,
                                      cursiv::ValidationAcceptor& accept) const {
  checkTarget(node, accept);
}
void VerilogValidator::checkStatement(const ast::AssignStmt& node,
                                     cursiv::ValidationAcceptor& accept) const {
  checkTarget(node, accept);
}
void VerilogValidator::checkProcedural(const ast::ProcAssignStmt& node,
                                      cursiv::ValidationAcceptor& accept) const {
  checkTarget(node, accept);
  const bool needsValue = node.kind == "assign" || node.kind == "force";
  if (needsValue != (node.value != nullptr))
    accept.error(node, needsValue ? "This procedural assignment requires a value"
                                  : "This operation does not take a value",
                 "verilog.procedural-value");
}
void VerilogValidator::checkCall(const ast::CallExpr& node,
                                cursiv::ValidationAcceptor& accept) const {
  // Task enables may leave input slots unconnected; function expressions may not.
  if (const auto* statement = cursiv::ast_cast<ast::TaskEnableStmt>(node.parent);
      statement && statement->call == &node) return;
  const auto* callee = cursiv::ast_cast<ast::HierIdent>(node.callee);
  if (!callee || callee->name.starts_with("$") || callee->name.starts_with("`")) return;
  for (auto arg : node.args)
    if (cursiv::ast_cast<ast::OmittedArg>(arg)) {
      accept.error(node, "A function call cannot omit an argument", "verilog.call-argument");
      return;
    }
}
void VerilogValidator::checkFunction(const ast::FunctionDecl& node,
                                    cursiv::ValidationAcceptor& accept) const {
  unsigned ports = 0;
  bool nonInput = false;
  auto count = [&](const ast::PortDecl* port) {
    if (!port) return;
    ports += port->items.size();
    nonInput |= port->direction != "input";
  };
  for (auto port : node.ansiPorts) count(port);
  for (auto item : node.items) count(cursiv::ast_cast<ast::PortDecl>(item));
  if (ports == 0)
    accept.error(node, "A Verilog-2005 function requires at least one input port",
                 "verilog.function-input");
  if (nonInput)
    accept.error(node, "Verilog-2005 function ports must be inputs",
                 "verilog.function-direction");
}
void VerilogValidator::checkTiming(const ast::SystemTimingCheck& node,
                                  cursiv::ValidationAcceptor& accept) const {
  unsigned min = 3, max = 4;
  if (node.kind == "$period") { min = 2; max = 3; }
  else if (node.kind == "$width") { min = 2; max = 4; }
  else if (node.kind == "$setuphold" || node.kind == "$recrem") { min = 4; max = 9; }
  else if (node.kind == "$timeskew") max = 6;
  else if (node.kind == "$fullskew") { min = 4; max = 7; }
  else if (node.kind == "$nochange") { min = 4; max = 5; }
  bool missing = node.args.size() < min;
  for (unsigned i = 0; i < min && i < node.args.size(); ++i)
    missing |= !node.args[i] || !node.args[i]->value;
  if (missing || node.args.size() > max)
    accept.error(node, "The timing check has missing or excess arguments",
                 "verilog.timing-arguments");
}
void VerilogValidator::checkInitial(const ast::InitialConstruct& node,
                                   cursiv::ValidationAcceptor& accept) const {
  if (cursiv::ast_cast<ast::NullStmt>(node.body))
    accept.error(node, "An initial construct requires a statement; use begin/end for an empty body",
                 "verilog.initial-statement");
}
void VerilogValidator::checkAttribute(const ast::AttributeInstance& node,
                                     cursiv::ValidationAcceptor& accept) const {
  for (const auto* parent = node.parent; parent; parent = parent->parent)
    if (cursiv::ast_cast<ast::AttributeSpec>(parent)) {
      accept.error(node, "An attribute value cannot contain another attribute instance.",
                   "verilog.nested-attribute");
      return;
    }
}
void register_validation_checks(VerilogCoreServices& services) {
  services.validator = std::make_unique<const VerilogValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&VerilogValidator::checkAssignment>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&VerilogValidator::checkStatement>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&VerilogValidator::checkProcedural>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&VerilogValidator::checkCall>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&VerilogValidator::checkFunction>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&VerilogValidator::checkTiming>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&VerilogValidator::checkInitial>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&VerilogValidator::checkAttribute>(*services.validator)});
}

}  // namespace verilog
