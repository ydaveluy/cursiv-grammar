#include "sdl/core/validation/SdlValidator.hpp"

#include <memory>
#include <string>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "sdl/core/CoreServices.hpp"

namespace sdl {

namespace {
std::string folded(std::string_view value) {
  std::string result(value);
  for (char& c : result) if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
  return result;
}
void checkEndName(const auto& node, cursiv::ValidationAcceptor& accept) {
  if (!node.endName.empty() && folded(node.name) != folded(node.endName))
    accept.error(node, "The ending name does not match its declaration.", "sdl.end-name");
}
}
void SdlValidator::checkModule(const ast::Module& node, cursiv::ValidationAcceptor& accept) const { checkEndName(node, accept); }
void SdlValidator::checkAggregate(const ast::AggregateStmt& node, cursiv::ValidationAcceptor& accept) const { if (!node.fill_) checkEndName(node, accept); }
void SdlValidator::checkNestedAggregate(const ast::NestedAggregate& node, cursiv::ValidationAcceptor& accept) const { if (!node.fill_) checkEndName(node, accept); }
void SdlValidator::checkConditional(const ast::ConditionalStmt& node, cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string> languages, ending;
  for (const auto name : node.languages)
    if (!languages.insert(folded(name)).second)
      accept.error(node, "Duplicate language in IFLANGUAGE.", "sdl.language-duplicate");
  for (const auto name : node.endLanguages)
    if (!ending.insert(folded(name)).second)
      accept.error(node, "Duplicate ending language.", "sdl.language-duplicate");
  if (!ending.empty() && ending != languages)
    accept.error(node, "END_IFLANGUAGE must name the same languages.", "sdl.language-end");
}

void register_validation_checks(SdlCoreServices& services) {
  using V = SdlValidator;
  services.validator = std::make_unique<const V>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkModule>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkAggregate>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkNestedAggregate>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkConditional>(validator)});
}

}  // namespace sdl
