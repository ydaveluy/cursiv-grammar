#include "atl/core/validation/AtlValidator.hpp"

#include <memory>
#include <string>
#include <unordered_set>

#include "atl/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace atl {

void AtlValidator::checkRuleNotSelfExtending(
    const ast::MatchedRule& rule, cursiv::ValidationAcceptor& accept) const {
  if (!rule.extendsRule.empty() && rule.extendsRule == rule.name)
    accept.error(rule, "A rule cannot extend itself.")
        .property<&ast::MatchedRule::extendsRule>();
}

void AtlValidator::checkModifiers(const ast::MatchedRule& rule, cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string_view> seen;
  for (std::size_t i = 0; i < rule.modifiers.size(); ++i)
    if (!seen.insert(rule.modifiers[i]).second)
      accept.error(rule, "Duplicate rule modifier: " + std::string(rule.modifiers[i]))
          .property<&ast::MatchedRule::modifiers>(i);
}

void register_validation_checks(AtlCoreServices& services) {
  using V = AtlValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkRuleNotSelfExtending>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkModifiers>(validator)});
}

}  // namespace atl
