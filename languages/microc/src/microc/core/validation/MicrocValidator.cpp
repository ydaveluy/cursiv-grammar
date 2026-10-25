#include "microc/core/validation/MicrocValidator.hpp"

#include <memory>
#include "cursiv/core/validation/ValidationRegistry.hpp"

#include "microc/core/CoreServices.hpp"

namespace microc {

void MicrocValidator::checkIdent(const ast::Ident& node,
                                 cursiv::ValidationAcceptor& accept) const {
  if (node.name == "if" || node.name == "else" || node.name == "while")
    accept.error(node, "A reserved word cannot be used as a name.", "microc.reserved-name")
        .property<&ast::Ident::name>();
}

void register_validation_checks(MicrocCoreServices& services) {
  services.validator = std::make_unique<const MicrocValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&MicrocValidator::checkIdent>(
          *services.validator)});
}

}  // namespace microc
