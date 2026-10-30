#include "hypertalk/core/validation/HypertalkValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "hypertalk/core/CoreServices.hpp"
#include "hypertalk/core/HypertalkGrammar.hpp"

namespace hypertalk {

// HyperTalk is case-insensitive, so `on mouseUp ... end mouseup` is a match
// the grammar cannot express (the closing name is read as free text, not
// compared to the opener).
void HypertalkValidator::checkHandlerNameMatches(
    const ast::Handler& handler, cursiv::ValidationAcceptor& accept) const {
  if (!ci_eq(handler.name, handler.endName))
    accept.error(handler, "`end` name does not match the handler's name.")
        .property<&ast::Handler::endName>();
}

void HypertalkValidator::checkFunctionNameMatches(
    const ast::Function& function, cursiv::ValidationAcceptor& accept) const {
  if (!ci_eq(function.name, function.endName))
    accept.error(function, "`end` name does not match the function's name.")
        .property<&ast::Function::endName>();
}

void HypertalkValidator::checkVisualUnlock(const ast::ScreenLockCmd& command,
                                          cursiv::ValidationAcceptor& accept) const {
  if (command.visualEffect && (command.lock || command.target != ast::LockTarget::Screen))
    accept.error(command, "A visual effect applies only to unlock screen.",
                 "hypertalk.visual-unlock");
}
void HypertalkValidator::checkShowCards(const ast::ShowHideCmd& command,
                                       cursiv::ValidationAcceptor& accept) const {
  if (!command.show && (command.allCards || command.markedCards || command.cardCount))
    accept.error(command, "A card display sequence uses show.",
                 "hypertalk.show-cards");
}

void register_validation_checks(HypertalkCoreServices& services) {
  using V = HypertalkValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkHandlerNameMatches>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkFunctionNameMatches>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkVisualUnlock>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkShowCards>(validator)});
}

}  // namespace hypertalk
