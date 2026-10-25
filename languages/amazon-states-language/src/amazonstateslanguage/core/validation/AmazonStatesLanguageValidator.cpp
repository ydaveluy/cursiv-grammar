#include "amazonstateslanguage/core/validation/AmazonStatesLanguageValidator.hpp"

#include <memory>
#include <string>
#include <unordered_set>
#include <type_traits>

#include "amazonstateslanguage/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace amazonstateslanguage {

template <class T>
void AmazonStatesLanguageValidator::checkMembers(
    const T &object, cursiv::ValidationAcceptor &accept) const {
  std::unordered_set<std::string_view> seen;
  for (std::size_t i = 0; i < object.members.size(); ++i)
    if (!seen.insert(object.members[i]).second)
      accept
          .error(object,
                 "Duplicate object member: " + std::string(object.members[i]))
          .template property<&T::members>(i);
  if constexpr (std::is_same_v<T, ast::ChoiceRule>) {
    if (const auto* rule = cursiv::ast_cast<ast::ComparisonRule>(&object)) {
      if (rule->variable.size() != 1 || rule->comparison_op.size() != 1)
        accept.error(object, "A comparison rule requires one Variable and one comparison operator.");
    } else if (const auto* rule = cursiv::ast_cast<ast::CompositeRule>(&object)) {
      if (rule->logical_op.size() != 1)
        accept.error(object, "A composite rule requires one logical operator.");
    }
  }

}

void register_validation_checks(AmazonStatesLanguageCoreServices &services) {
  using V = AmazonStatesLanguageValidator;
  services.validator = std::make_unique<const V>(services);
  const V &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkMembers<ast::ReaderConfig>>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkMembers<ast::ItemReader>>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkMembers<ast::ProcessorConfig>>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkMembers<ast::Catcher>>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkMembers<ast::ChoiceRule>>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkMembers<ast::Program>>(validator)});
}

} // namespace amazonstateslanguage
