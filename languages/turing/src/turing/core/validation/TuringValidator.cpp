#include "turing/core/validation/TuringValidator.hpp"

#include <memory>
#include <string>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "turing/core/CoreServices.hpp"

namespace turing {

void TuringValidator::checkEnumNames(
    const ast::EnumType& enumeration, cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string_view> names;
  for (std::size_t i = 0; i < enumeration.names.size(); ++i)
    if (!names.insert(enumeration.names[i]).second)
      accept.error(enumeration, "Enumerated identifiers must be distinct.",
                   "turing.enum-duplicate").property<&ast::EnumType::names>(i);
}

void TuringValidator::checkSubprogramEndNameMatches(
    const ast::SubprogramDecl& subprogram,
    cursiv::ValidationAcceptor& accept) const {
  if (!subprogram.endName.empty() && subprogram.endName != subprogram.name)
    accept
        .error(subprogram, "'end " + std::string(subprogram.endName) +
                                "' does not match '" +
                                std::string(subprogram.name) + "'")
        .property<&ast::SubprogramDecl::endName>();
}

void TuringValidator::checkCaseLabels(const ast::CaseStmt& statement,
                                       cursiv::ValidationAcceptor& accept) const {
  for (std::size_t i = 0; i < statement.labels.size(); ++i)
    if (statement.labels[i]->values.empty() && i + 1 != statement.labels.size())
      accept.error(*statement.labels[i], "The default label must be last.");
}

void register_validation_checks(TuringCoreServices& services) {
  using V = TuringValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkSubprogramEndNameMatches>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkCaseLabels>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkEnumNames>(validator)});
}

}  // namespace turing
