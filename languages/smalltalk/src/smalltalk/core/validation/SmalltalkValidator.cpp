#include "smalltalk/core/validation/SmalltalkValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "smalltalk/core/CoreServices.hpp"

namespace smalltalk {

void SmalltalkValidator::checkDictionary(const ast::DynamicDictionary& node,
                                        cursiv::ValidationAcceptor& accept) const {
  for (const auto* item : node.items) {
    const auto* send = cursiv::ast_cast<ast::MessageSend>(item);
    if (!send || send->selectorParts.size() != 1 ||
        send->selectorParts[0] != "->" || send->args.size() != 1)
      accept.error(node, "A dynamic dictionary item must be an association send.",
                   "smalltalk.dictionary-association");
  }
}

void SmalltalkValidator::checkCascade(const ast::Cascade& node,
                                     cursiv::ValidationAcceptor& accept) const {
  if (!cursiv::ast_cast<ast::MessageSend>(node.base))
    accept.error(node, "A cascade must start with a message send.",
                 "smalltalk.cascade-send");
}

void register_validation_checks(SmalltalkCoreServices& services) {
  services.validator = std::make_unique<const SmalltalkValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<
          &SmalltalkValidator::checkDictionary>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<
          &SmalltalkValidator::checkCascade>(*services.validator)});
}

}  // namespace smalltalk
