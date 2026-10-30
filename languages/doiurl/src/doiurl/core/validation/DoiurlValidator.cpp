#include "doiurl/core/validation/DoiurlValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "doiurl/core/CoreServices.hpp"

namespace doiurl {

void DoiurlValidator::checkName(const ast::DoiUri& node,
                               cursiv::ValidationAcceptor& accept) const {
  if (node.prefix.empty())
    accept.error(node, "A DOI prefix must not be empty.", "doi.prefix")
        .property<&ast::DoiUri::prefix>();
  // Slashes belong to the suffix, so two empty segments still spell '/'.
  if (node.suffix.empty() ||
      (node.suffix.size() == 1 && node.suffix[0].empty()))
    accept.error(node, "A DOI suffix must not be empty.", "doi.suffix");
}

void register_validation_checks(DoiurlCoreServices& services) {
  services.validator = std::make_unique<const DoiurlValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<
          &DoiurlValidator::checkName>(*services.validator)});
}

}  // namespace doiurl
