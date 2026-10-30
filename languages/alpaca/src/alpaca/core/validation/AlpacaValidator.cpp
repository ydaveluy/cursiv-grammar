#include "alpaca/core/validation/AlpacaValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "alpaca/core/CoreServices.hpp"

namespace alpaca {

void AlpacaValidator::checkAdjacency(
    const ast::AdjacencyPred& node,
    cursiv::ValidationAcceptor& accept) const {
  if (node.count.find_first_not_of('0') == std::string_view::npos)
    accept.error(node, "An adjacency count must be at least one.")
        .property<&ast::AdjacencyPred::count>();
}

void register_validation_checks(AlpacaCoreServices& services) {
  services.validator = std::make_unique<const AlpacaValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<
          &AlpacaValidator::checkAdjacency>(*services.validator)});
}

}  // namespace alpaca
