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
  for (std::size_t begin = 0; begin < ipv4.size();) {
    const auto separator = ipv4.find('.', begin);
    const auto end = separator == std::string_view::npos ? ipv4.size() : separator;
    const auto octet = ipv4.substr(begin, end - begin);
    unsigned value = 0;
    for (const char c : octet) value = value * 10 + static_cast<unsigned>(c - '0');
    if (value > 255 || (octet.size() > 1 && octet[0] == '0')) {
      accept
          .error(authority, "An IPv4 octet must be between 0 and 255 without leading zeros.",
                 "iri.address")
          .property<&ast::Authority::raw>();
      return;
    }
    begin = end + 1;
  }
}

void register_validation_checks(IriCoreServices& services) {
  services.validator = std::make_unique<const IriValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &IriValidator::checkAuthority>(*services.validator)});
}

}  // namespace iri
