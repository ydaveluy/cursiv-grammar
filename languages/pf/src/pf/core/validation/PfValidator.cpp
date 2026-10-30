#include "pf/core/validation/PfValidator.hpp"

#include <memory>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "pf/core/CoreServices.hpp"

namespace pf {

void PfValidator::checkFile(const ast::File& file,
                            cursiv::ValidationAcceptor& accept) const {
  std::vector<const ast::AnchorRule*> stack;
  for (const auto& line : file.lines) {
    if (const auto* anchor = std::get_if<ast::AnchorRule*>(&line)) {
      if (*anchor && (*anchor)->opensBlock) stack.push_back(*anchor);
    } else if (const auto* close = std::get_if<ast::AnchorClose*>(&line)) {
      if (!*close) continue;
      if (stack.empty()) accept.error(**close, "No inline anchor is open.", "pf.anchor-close");
      else stack.pop_back();
    }
  }
  for (const auto* anchor : stack)
    accept.error(*anchor, "The inline anchor has no closing brace.", "pf.anchor-open");
}
void PfValidator::checkOption(const ast::Option& option,
                              cursiv::ValidationAcceptor& accept) const {
  if (option.kind.empty() ||
      ((option.kind == "optimization" || option.kind == "ruleset-optimization") &&
       option.word.empty()))
    accept.error(option, "The set directive requires an option and its argument.",
                 "pf.set-argument");
}

void register_validation_checks(PfCoreServices& services) {
  services.validator = std::make_unique<const PfValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&PfValidator::checkFile>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&PfValidator::checkOption>(*services.validator)});
}

}  // namespace pf
