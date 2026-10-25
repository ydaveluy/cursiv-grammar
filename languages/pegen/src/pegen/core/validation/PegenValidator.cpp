#include "pegen/core/validation/PegenValidator.hpp"

#include <memory>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "pegen/core/CoreServices.hpp"

namespace pegen {

void PegenValidator::checkGrammar(
    const ast::Grammar& grammar, cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string_view> names;
  for (const auto* rule : grammar.rules) {
    if (rule && !names.insert(rule->name).second)
      accept.error(*rule, "Rule is already defined.", "pegen.duplicateRule")
          .property<&ast::Rule::name>();
  }
}

void register_validation_checks(PegenCoreServices& services) {
  services.validator = std::make_unique<const PegenValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&PegenValidator::checkGrammar>(
          *services.validator)});
}

}  // namespace pegen
