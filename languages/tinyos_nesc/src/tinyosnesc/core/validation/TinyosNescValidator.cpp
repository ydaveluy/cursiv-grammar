#include "tinyosnesc/core/validation/TinyosNescValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "tinyosnesc/core/CoreServices.hpp"

namespace tinyosnesc {

void TinyosNescValidator::checkModule(const ast::Module& n, cursiv::ValidationAcceptor& a) const {
  if (n.isGeneric != n.hasParameters)
    a.error(n, "A generic component has a parameter list; a non-generic one does not.", "tinyosnesc.generic-parameters");
}
void TinyosNescValidator::checkConfiguration(const ast::Configuration& n, cursiv::ValidationAcceptor& a) const {
  if (n.isGeneric != n.hasParameters)
    a.error(n, "A generic component has a parameter list; a non-generic one does not.", "tinyosnesc.generic-parameters");
}
void TinyosNescValidator::checkDocument(const ast::Document& n, cursiv::ValidationAcceptor& a) const {
  if (n.components.size() != 1)
    a.error(n, "A nesC source file defines one interface or component.", "tinyosnesc.source-definition");
}

void register_validation_checks(TinyosNescCoreServices& services) {
  services.validator =
      std::make_unique<const TinyosNescValidator>(services);
  using V = TinyosNescValidator;
  const auto& v = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkModule>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkConfiguration>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkDocument>(v)});
}

}  // namespace tinyosnesc
