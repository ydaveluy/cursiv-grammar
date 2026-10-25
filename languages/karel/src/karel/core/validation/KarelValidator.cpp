#include "karel/core/validation/KarelValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "karel/core/CoreServices.hpp"

namespace karel {

void KarelValidator::checkUniqueDefinitions(
    const ast::Program& program, cursiv::ValidationAcceptor& accept) const {
  std::unordered_map<std::string_view, std::vector<const ast::Definition*>>
      names;
  for (const ast::Definition* const definition : program.definitions)
    if (definition != nullptr) names[definition->name].push_back(definition);

  for (const auto& [name, definitions] : names) {
    if (definitions.size() <= 1) continue;
    const std::string message = "Duplicate definition: " + std::string(name);
    for (const ast::Definition* const definition : definitions)
      accept.error(*definition, message).property<&ast::Definition::name>();
  }
}

void register_validation_checks(KarelCoreServices& services) {
  using V = KarelValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkUniqueDefinitions>(validator)});
}

}  // namespace karel
