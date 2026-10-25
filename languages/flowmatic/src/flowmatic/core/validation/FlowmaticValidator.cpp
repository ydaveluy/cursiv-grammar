#include "flowmatic/core/validation/FlowmaticValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "flowmatic/core/CoreServices.hpp"

namespace flowmatic {

void FlowmaticValidator::checkArithmetic(
    const ast::ArithmeticStmt &node, cursiv::ValidationAcceptor &accept) const {
  const bool product = node.op == ast::ArithmeticOp::multiply ||
                       node.op == ast::ArithmeticOp::divide;
  if (product && !node.result)
    accept.error(node, "Expected a GIVING field.",
                 "flowmatic.arithmetic-result");
  if (node.op != ast::ArithmeticOp::add && node.left && node.left->file.empty())
    accept
        .error(node, "Expected a field reference as the first operand.",
               "flowmatic.arithmetic-left")
        .property<&ast::ArithmeticStmt::left>();
  if (!product && node.right && node.right->file.empty())
    accept
        .error(node, "Expected a field reference as the destination operand.",
               "flowmatic.arithmetic-right")
        .property<&ast::ArithmeticStmt::right>();
}

void register_validation_checks(FlowmaticCoreServices &services) {
  services.validator = std::make_unique<const FlowmaticValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &FlowmaticValidator::checkArithmetic>(*services.validator)});
}

} // namespace flowmatic
