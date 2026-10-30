#include "rexx/core/validation/RexxValidator.hpp"

#include <memory>
#include <string>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "rexx/core/CoreServices.hpp"

namespace rexx {

namespace {
std::string folded(std::string_view value) {
  std::string result(value);
  for (char& c : result) if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
  return result;
}
}
void RexxValidator::checkAssignment(const ast::Assignment& node,
                                    cursiv::ValidationAcceptor& accept) const {
  if (!node.target.empty() && ((node.target.front() >= '0' && node.target.front() <= '9') ||
                               node.target.front() == '.'))
    accept.error(node, "Assignment requires a variable symbol, not a constant symbol.",
                 "rexx.assignment-target").property<&ast::Assignment::target>();
}

void RexxValidator::checkDoClauses(const ast::DoStmt& node,
                                   cursiv::ValidationAcceptor& accept) const {
  if (!node.endName.empty() && (!node.counter ||
      folded(node.endName) != folded(node.counter->target)))
    accept.error(node, "The END name must match the controlled DO variable.", "rexx.end-name")
        .property<&ast::DoStmt::endName>();
  if (node.counter && !node.counter->value)
    accept.error(*node.counter, "A controlled DO needs an initial expression.", "rexx.do-initial");
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
      {cursiv::ValidationRegistry::makeValidationCheck<&RexxValidator::checkAssignment>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
          &RexxValidator::checkDoClauses>(*services.validator)});
}

}  // namespace rexx
