#include "rfc1035/core/validation/Rfc1035Validator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "rfc1035/core/CoreServices.hpp"
#include "rfc1035/core/Label.hpp"

namespace rfc1035 {

void Rfc1035Validator::checkDomain(
    const ast::Domain& domain, cursiv::ValidationAcceptor& accept) const {
  const auto& labels = domain.labels;
  if (labels.size() == 1 && (labels[0].empty() || labels[0] == "@")) return;
  if (labels.size() == 2 && labels[0].empty() && labels[1].empty()) return;
  std::size_t encoded = 0;
  for (std::size_t i = 0; i < labels.size(); ++i) {
    const auto value = label_value(labels[i]);
    if (!value) {
      accept.error(domain, "A label escape needs a character or three decimal digits from 000 to 255.",
                   "rfc1035.escape").property<&ast::Domain::labels>();
      continue;
    }
    if (value->empty() && i + 1 != labels.size())
      accept.error(domain, "An empty label is only allowed at the root dot.",
                   "rfc1035.empty-label").property<&ast::Domain::labels>();
    if (value->size() > 63)
      accept.error(domain, "A decoded label exceeds 63 octets.",
                   "rfc1035.label-length").property<&ast::Domain::labels>();
    encoded += value->size() + 1;
  }
  if (encoded > 255)
    accept.error(domain, "The encoded name exceeds 255 octets.",
                 "rfc1035.name-length").property<&ast::Domain::labels>();
}

void register_validation_checks(Rfc1035CoreServices& services) {
  services.validator = std::make_unique<const Rfc1035Validator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&Rfc1035Validator::checkDomain>(
          *services.validator)});
}

}  // namespace rfc1035
