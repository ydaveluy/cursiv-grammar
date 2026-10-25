#include "rfc822/core/validation/Rfc822Validator.hpp"

#include <memory>

#include "cursiv/core/syntax-tree/CstUtils.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "rfc822/core/CoreServices.hpp"

namespace rfc822 {

void Rfc822Validator::checkGroup(
    const ast::Group& group, cursiv::ValidationAcceptor& accept) const {
  const auto* document = group.getDocument();
  if (!document || group.cstNodeId == cursiv::kNoNode) return;
  const auto& cst = document->parseResult.cst();
  const auto commas = cursiv::find_nodes_for_keyword({&cst, group.cstNodeId}, ",");
  for (std::size_t i = 1; i < group.mailboxes.size(); ++i) {
    const auto* previous = group.mailboxes[i - 1];
    const auto* current = group.mailboxes[i];
    if (!previous || !current || previous->cstNodeId == cursiv::kNoNode ||
        current->cstNodeId == cursiv::kNoNode) continue;
    bool separated = false;
    for (const auto& comma : commas)
      separated |= comma.begin() >= cst[previous->cstNodeId].end &&
                   comma.end() <= cst[current->cstNodeId].begin;
    if (!separated)
      accept.error(group, "Expected ',' between group mailboxes.", "rfc822.separator")
          .property<&ast::Group::mailboxes>(i);
  }
}

void register_validation_checks(Rfc822CoreServices& services) {
  using V = Rfc822Validator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkGroup>(
          *services.validator)});
}

}  // namespace rfc822
