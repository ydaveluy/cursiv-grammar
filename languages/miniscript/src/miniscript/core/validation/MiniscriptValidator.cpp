#include "miniscript/core/validation/MiniscriptValidator.hpp"

#include <memory>
#include <string>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "miniscript/core/CoreServices.hpp"

namespace miniscript {
namespace {
std::string_view normalized(std::string_view value) {
  const auto first = value.find_first_not_of('0');
  return first == std::string_view::npos ? std::string_view{} : value.substr(first);
}

bool exceeds_count(std::string_view threshold, std::size_t count) {
  threshold = normalized(threshold);
  const auto limit = std::to_string(count);
  return threshold.size() > limit.size() ||
         (threshold.size() == limit.size() && threshold > limit);
}
}  // namespace

void MiniscriptValidator::checkMultisig(
    const ast::MultisigExpression& expr,
    cursiv::ValidationAcceptor& accept) const {
  if (!expr.isMultiA && expr.keys.size() > 20)
    accept.error(expr, "multi permits at most 20 keys.");
  if (normalized(expr.threshold).empty())
    accept.error(expr, "The threshold must be positive.", "miniscript.threshold")
        .property<&ast::MultisigExpression::threshold>();
  if (exceeds_count(expr.threshold, expr.keys.size()))
    accept.error(expr, "The threshold must not exceed the number of keys.")
        .property<&ast::MultisigExpression::threshold>();
}

void MiniscriptValidator::checkThreshold(
    const ast::ThresholdExpression& expr,
    cursiv::ValidationAcceptor& accept) const {
  if (normalized(expr.threshold).empty())
    accept.error(expr, "The threshold must be positive.", "miniscript.threshold")
        .property<&ast::ThresholdExpression::threshold>();
  if (exceeds_count(expr.threshold, expr.expressions.size()))
    accept.error(expr, "The threshold must not exceed the number of expressions.")
        .property<&ast::ThresholdExpression::threshold>();
}

void MiniscriptValidator::checkTime(
    const ast::TimeExpression& expr,
    cursiv::ValidationAcceptor& accept) const {
  const auto value = normalized(expr.value);
  if (value.empty() || value.size() > 10 ||
      (value.size() == 10 && value > "2147483647"))
    accept.error(expr, "The locktime must be between 1 and 2147483647.",
                 "miniscript.locktime")
        .property<&ast::TimeExpression::value>();
}

void register_validation_checks(MiniscriptCoreServices& services) {
  services.validator = std::make_unique<const MiniscriptValidator>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &MiniscriptValidator::checkMultisig>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
          &MiniscriptValidator::checkThreshold>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
          &MiniscriptValidator::checkTime>(validator)});
}

}  // namespace miniscript
