#include "apt/core/validation/AptValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "apt/core/CoreServices.hpp"
#include "apt/core/Word.hpp"

namespace apt {

void AptValidator::checkRecord(const ast::Record& record,
                               cursiv::ValidationAcceptor& accept) const {
  if (word_value(record.uri).find(':') == std::string::npos)
    accept.error(record, "The repository URI needs a scheme followed by ':'.",
                 "apt.uri").property<&ast::Record::uri>();
  const auto suite = word_value(record.distribution);
  if (suite.ends_with('/')) {
    if (!record.components.empty())
      accept.error(record, "An absolute suite cannot have components.",
                   "apt.components").property<&ast::Record::components>();
  } else if (record.components.empty()) {
    accept.error(record, "The repository needs at least one component.",
                 "apt.components").property<&ast::Record::distribution>();
  }
}

void register_validation_checks(AptCoreServices& services) {
  services.validator = std::make_unique<const AptValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&AptValidator::checkRecord>(
          *services.validator)});
}

}  // namespace apt
