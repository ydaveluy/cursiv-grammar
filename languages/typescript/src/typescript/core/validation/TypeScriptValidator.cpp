#include "typescript/core/validation/TypeScriptValidator.hpp"

#include <memory>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "typescript/core/CoreServices.hpp"

namespace typescript {
namespace {
bool memberSeparator(std::string_view gap) {
  if (gap.find_first_of("\r\n") != gap.npos ||
      gap.find("\xE2\x80\xA8") != gap.npos ||
      gap.find("\xE2\x80\xA9") != gap.npos)
    return true;
  for (std::size_t i = 0; i < gap.size(); ++i) {
    if (gap.substr(i).starts_with("/*")) {
      const auto end = gap.find("*/", i + 2);
      if (end == gap.npos) return false;
      i = end + 1;
    } else if (gap.substr(i).starts_with("//")) {
      return false;
    } else if (gap[i] == ';' || gap[i] == ',') {
      return true;
    }
  }
  return false;
}

template <class Members>
void checkMembers(const Members& members, cursiv::ValidationAcceptor& accept) {
  for (std::size_t i = 1; i < members.size(); ++i) {
    const auto* previous = members[i - 1];
    const auto* current = members[i];
    if (!previous || !current) continue;
    const auto* document = current->getDocument();
    if (!document || previous->cstNodeId == cursiv::kNoNode ||
        current->cstNodeId == cursiv::kNoNode)
      continue;
    const auto& cst = document->parseResult.cst();
    const auto begin = cst[previous->cstNodeId].end;
    const auto end = cst[current->cstNodeId].begin;
    if (begin <= end && !memberSeparator(cst.text().substr(begin, end - begin)))
      accept.error(*current, "Expected ';', ',' or a line break between type members.",
                   "typescript.type-member-separator")
          .template property<&ast::TypeMember::key>();
  }
}
}  // namespace

void TypeScriptValidator::checkObjectMembers(
    const ast::ObjectType& object, cursiv::ValidationAcceptor& accept) const {
  checkMembers(object.members, accept);
}

void TypeScriptValidator::checkInterfaceMembers(
    const ast::InterfaceDecl& interface, cursiv::ValidationAcceptor& accept) const {
  checkMembers(interface.members, accept);
}

void register_validation_checks(TypeScriptCoreServices& services) {
  using V = TypeScriptValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkObjectMembers>(
          *services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkInterfaceMembers>(
          *services.validator)});
}

}  // namespace typescript
