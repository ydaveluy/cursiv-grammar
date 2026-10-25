#include "lolcode/core/validation/LolcodeValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "lolcode/core/CoreServices.hpp"

namespace lolcode {

// The grammar reads two independent LABELs (`IM IN YR x ... IM OUTTA YR y`);
// the language requires them to name the same loop.
void LolcodeValidator::checkLoopLabelsMatch(
    const ast::LoopStmt& loop, cursiv::ValidationAcceptor& accept) const {
  if (loop.label != loop.endLabel)
    accept.error(loop, "Loop closing label must match its opening label.")
        .property<&ast::LoopStmt::endLabel>();
}

void register_validation_checks(LolcodeCoreServices& services) {
  using V = LolcodeValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkLoopLabelsMatch>(validator)});
}

}  // namespace lolcode
