#include "wln/core/validation/WlnValidator.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "wln/core/CoreServices.hpp"
#include <memory>
namespace wln {
void WlnValidator::checkGroup(const ast::Group &node,
                              cursiv::ValidationAcceptor &accept) const {
  if (node.separator.size() > 1)
    accept
        .error(node, "WLN uses a single separator space.",
               "wln.separator-space")
        .property<&ast::Group::separator>();
  for (auto symbol : node.symbols) {
    if (!symbol.empty() && symbol[0] == '0')
      accept
          .error(node, "A chain length cannot start with zero.",
                 "wln.chain-length")
          .property<&ast::Group::symbols>();
    for (char c : symbol)
      if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '&' ||
            c == '-' || c == '*')) {
        accept
            .error(node, "Unexpected character in WLN notation.", "wln.symbol")
            .property<&ast::Group::symbols>();
        return;
      }
  }
}
void WlnValidator::checkRecord(const ast::Record &node,
                               cursiv::ValidationAcceptor &accept) const {
  if (!node.trailing.empty())
    accept
        .error(node, "WLN notation cannot end with a separator space.",
               "wln.trailing-space")
        .property<&ast::Record::trailing>();
}
void register_validation_checks(WlnCoreServices &services) {
  services.validator = std::make_unique<const WlnValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &WlnValidator::checkGroup>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &WlnValidator::checkRecord>(*services.validator)});
}
} // namespace wln
