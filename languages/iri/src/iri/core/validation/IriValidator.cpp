#include "iri/core/validation/IriValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "iri/core/CoreServices.hpp"

namespace iri {

void IriValidator::checkAuthority(const ast::Authority& authority,
                                  cursiv::ValidationAcceptor& accept) const {
  const auto open = authority.raw.find('[');
  if (open == std::string_view::npos || open + 1 == authority.raw.size())
    return;
  const auto close = authority.raw.find(']', open);
  if (close == std::string_view::npos) return;
  const auto address = authority.raw.substr(open + 1, close - open - 1);
  if (address.empty() || address.front() == 'v' || address.front() == 'V' ||
      address.find('.') == std::string_view::npos)
    return;
  const auto ipv4 = address.substr(address.rfind(':') + 1);
  unsigned value = 0;
  for (const char c : ipv4) {
    if (c == '.')
      value = 0;
    else
      value = value * 10 + static_cast<unsigned>(c - '0');
    if (value > 255) {
      accept
          .error(authority, "An IPv4 octet must be between 0 and 255.",
                 "iri.address")
          .property<&ast::Authority::raw>();
      return;
    }
  }
}

void register_validation_checks(IriCoreServices& services) {
  services.validator = std::make_unique<const IriValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &IriValidator::checkAuthority>(*services.validator)});
}

}  // namespace iri
