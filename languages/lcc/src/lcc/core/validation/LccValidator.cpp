#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "lcc/core/validation/LccValidator.hpp"

#include <memory>

#include "lcc/core/CoreServices.hpp"

namespace lcc {

void LccValidator::checkClass(const ast::CallNumber& number,
                              cursiv::ValidationAcceptor& accept) const {
  if (number.subclasses.empty() || number.subclasses[0].size() > 4 ||
      number.subclasses.size() > 2)
    accept.error(number, "An LC class number has one to four whole-number digits and at most one decimal part.",
                 "lcc.class-number").property<&ast::CallNumber::subclasses>();
  if (number.topic.size() + (number.subtopic ? number.subtopic->size() : 0) > 3)
    accept.error(number, "A class has at most three letters.", "lcc.class-letters")
        .property<&ast::CallNumber::subtopic>();
}

void LccValidator::checkCutter(const ast::Cutter& cutter,
                               cursiv::ValidationAcceptor& accept) const {
  if (cutter.letter.size() > 3)
    accept.error(cutter, "A Cutter has at most three letters.", "lcc.cutter-letters")
        .property<&ast::Cutter::letter>();
}

void register_validation_checks(LccCoreServices& services) {
  services.validator = std::make_unique<const LccValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&LccValidator::checkClass>(
          *services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&LccValidator::checkCutter>(
          *services.validator)});
}

}  // namespace lcc
