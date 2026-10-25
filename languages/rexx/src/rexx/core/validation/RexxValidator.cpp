#include "rexx/core/validation/RexxValidator.hpp"

#include <memory>
#include <string>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "rexx/core/CoreServices.hpp"

namespace rexx {

void RexxValidator::checkDoClauses(const ast::DoStmt& node,
                                   cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string> seen;
  for (const auto* clause : node.clauses) {
    if (!clause) continue;
    std::string name(clause->keyword);
    for (auto& c : name)
      if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    if (!seen.insert(name).second)
      accept.error(*clause, "Duplicate DO control clause.");
  }
}

void register_validation_checks(RexxCoreServices& services) {
  services.validator = std::make_unique<const RexxValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &RexxValidator::checkDoClauses>(*services.validator)});
}

}  // namespace rexx
