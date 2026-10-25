#include "dice/core/validation/DiceValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "dice/core/CoreServices.hpp"

namespace dice {

void DiceValidator::checkDivision(const ast::BinaryNotation &expression,
                                  cursiv::ValidationAcceptor &accept) const {
  if (expression.op != ast::Operator::divide)
    return;
  const auto *number = cursiv::ast_cast<ast::NumberLiteral>(expression.right);
  if (number && !number->value.empty() &&
      number->value.find_first_not_of('0') == std::string_view::npos)
    accept.error(expression, "Division by a zero literal.", "dice.zero-divisor")
        .property<&ast::BinaryNotation::right>();
}

void register_validation_checks(DiceCoreServices &services) {
  services.validator = std::make_unique<const DiceValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &DiceValidator::checkDivision>(*services.validator)});
}

} // namespace dice
