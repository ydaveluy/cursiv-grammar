#include "cto/core/validation/CtoValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cto/core/CoreServices.hpp"

namespace cto {

void CtoValidator::checkUniqueDeclarations(
    const ast::Model& model, cursiv::ValidationAcceptor& accept) const {
  std::unordered_map<std::string_view, std::vector<const ast::Declaration*>>
      names;
  for (const ast::Declaration* const decl : model.declarations)
    if (decl != nullptr) names[decl->name].push_back(decl);

  for (const auto& [name, decls] : names) {
    if (decls.size() <= 1) continue;
    const std::string message = "Duplicate declaration name: " + std::string(name);
    for (const ast::Declaration* const decl : decls)
      accept.error(*decl, message).property<&ast::Declaration::name>();
  }
}

void register_validation_checks(CtoCoreServices& services) {
  using V = CtoValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkUniqueDeclarations>(validator)});
}

}  // namespace cto
