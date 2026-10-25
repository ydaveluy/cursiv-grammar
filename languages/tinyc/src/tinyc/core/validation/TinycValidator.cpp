#include "tinyc/core/validation/TinycValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "tinyc/core/CoreServices.hpp"

namespace tinyc {
void TinycValidator::checkProgram(const ast::Program &program,
                                  cursiv::ValidationAcceptor &accept) const {
  if (program.statements.size() != 1)
    accept
        .error(program,
               "A Tiny-C program is one statement; use a block for several "
               "statements.",
               "tinyc.program")
        .property<&ast::Program::statements>(program.statements.empty() ? 0
                                                                        : 1);
}
void TinycValidator::checkIdentifier(const ast::Ident &identifier,
                                     cursiv::ValidationAcceptor &accept) const {
  if (identifier.name.size() != 1 || identifier.name[0] < 'a' ||
      identifier.name[0] > 'z')
    accept
        .error(identifier,
               "Tiny-C variables are the predefined names a through z.",
               "tinyc.variable")
        .property<&ast::Ident::name>();
}
void register_validation_checks(TinycCoreServices &services) {
  services.validator = std::make_unique<const TinycValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &TinycValidator::checkProgram>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &TinycValidator::checkIdentifier>(*services.validator)});
}
} // namespace tinyc
