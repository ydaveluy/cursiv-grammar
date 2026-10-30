#include "informix/core/validation/InformixValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/text/scalar_at.hpp"
#include "informix/core/CoreServices.hpp"

namespace informix {

void InformixValidator::checkLike(const ast::LikeExpr& like,
                                  cursiv::ValidationAcceptor& accept) const {
  if (!like.escape) return;
  const auto text = like.escape->value;
  std::size_t width = 0;
  if (!text.empty()) (void)cursiv::scalar_at(text.data(), text.size(), width);
  if (text.empty() || width != text.size())
    accept.error(*like.escape, "ESCAPE requires exactly one character.", "informix.escape");
}

void InformixValidator::checkMenuEvent(const ast::MenuEvent& event,
                                       cursiv::ValidationAcceptor& accept) const {
  if (event.keys.size() > 4)
    accept.error(event, "A menu command accepts at most four activation keys.", "informix.menu-keys");
  if (event.body.empty())
    accept.error(event, "A menu control block requires at least one statement.", "informix.menu-body");
}

void InformixValidator::checkMenu(const ast::MenuStmt& menu,
                                  cursiv::ValidationAcceptor& accept) const {
  for (const auto* event : menu.events)
    if (event && !event->beforeMenu && event->optionName) return;
  accept.error(menu, "A menu requires a visible option.", "informix.menu-options");
}

void register_validation_checks(InformixCoreServices& services) {
  using V = InformixValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkLike>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkMenuEvent>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkMenu>(*services.validator)});
}

}  // namespace informix
