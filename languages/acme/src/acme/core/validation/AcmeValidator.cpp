#include "acme/core/validation/AcmeValidator.hpp"

#include <memory>
#include <string>
#include <unordered_set>
#include "cursiv/core/validation/ValidationRegistry.hpp"

#include "acme/core/CoreServices.hpp"

namespace acme {

template<class T>
void AcmeValidator::checkModifiers(const T& declaration, cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string> seen;
  for (std::size_t i = 0; i < declaration.modifiers.size(); ++i) {
    std::string name(declaration.modifiers[i]);
    for (char& c : name) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    if (!seen.insert(name).second)
      accept.error(declaration, "Duplicate type modifier: " + name)
          .template property<&T::modifiers>(i);
  }
}

void register_validation_checks(AcmeCoreServices& services) {
  services.validator = std::make_unique<const AcmeValidator>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&AcmeValidator::checkModifiers<ast::PortTypeDecl>>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&AcmeValidator::checkModifiers<ast::RoleTypeDecl>>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&AcmeValidator::checkModifiers<ast::ComponentTypeDecl>>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&AcmeValidator::checkModifiers<ast::ConnectorTypeDecl>>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&AcmeValidator::checkModifiers<ast::ElementTypeDecl>>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&AcmeValidator::checkModifiers<ast::GroupTypeDecl>>(validator)});

}

}  // namespace acme
