#include "rust/core/validation/RustValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "rust/core/CoreServices.hpp"

namespace rust {

namespace {

bool comparison(ast::Operator op) {
  switch (op) {
    case ast::Operator::eq:
    case ast::Operator::ne:
    case ast::Operator::lt:
    case ast::Operator::gt:
    case ast::Operator::le:
    case ast::Operator::ge:
      return true;
    default:
      return false;
  }
}

}  // namespace

void RustValidator::checkComparisonChain(
    const ast::BinaryExpr& expression, cursiv::ValidationAcceptor& accept) const {
  const auto* left = cursiv::ast_cast<ast::BinaryExpr>(expression.left);
  if (comparison(expression.op) && left && comparison(left->op))
    accept.error(expression, "Comparison operators cannot be chained; "
                             "parenthesize the comparison.",
                 "comparison-chain")
        .property<&ast::BinaryExpr::op>();
}

void register_validation_checks(RustCoreServices& services) {
  using V = RustValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkComparisonChain>(
          *services.validator)});
}

}  // namespace rust
