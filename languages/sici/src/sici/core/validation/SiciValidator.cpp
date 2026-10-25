#include "sici/core/validation/SiciValidator.hpp"

#include <memory>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "sici/core/CoreServices.hpp"

namespace sici {

void SiciValidator::checkIssnChecksum(
    const ast::Item& item, cursiv::ValidationAcceptor& accept) const {
  char digits[8];
  int n = 0;
  for (const char c : item.issn)
    if (c != '-' && n < 8) digits[n++] = c;
  if (n != 8) return;

  int sum = 0;
  for (int i = 0; i < 7; ++i) sum += (digits[i] - '0') * (8 - i);
  const int want = (11 - sum % 11) % 11;
  const int actual =
      digits[7] == 'X' || digits[7] == 'x' ? 10 : digits[7] - '0';
  if (want != actual)
    accept.warning(item, "ISSN checksum does not verify.")
        .property<&ast::Item::issn>();
}

void SiciValidator::checkControl(const ast::Control& control,
                                 cursiv::ValidationAcceptor& accept) const {
  if (control.csi != "1" && control.csi != "2" && control.csi != "3")
    accept.warning(control, "SICI code structure identifier must be 1, 2 or 3.")
        .property<&ast::Control::csi>();
  if (control.dpi != "0" && control.dpi != "1" && control.dpi != "2" &&
      control.dpi != "3")
    accept.warning(control, "SICI derivative part identifier must be 0 to 3.")
        .property<&ast::Control::dpi>();
  constexpr std::string_view media[] = {"CD", "CF", "CO", "CT", "HD", "HE",
                                        "SC", "TB", "TH", "TL", "TS", "TX",
                                        "VX", "ZN", "ZU", "ZZ"};
  bool known = false;
  for (const auto medium : media) known |= control.mfi == medium;
  if (!known)
    accept.warning(control, "Unknown SICI medium/format identifier.")
        .property<&ast::Control::mfi>();
  if (control.version != "2")
    accept.warning(control, "Unsupported SICI version; expected 2.")
        .property<&ast::Control::version>();
}

void SiciValidator::checkContribution(
    const ast::Contribution& contribution,
    cursiv::ValidationAcceptor& accept) const {
  if (contribution.title.size() > 6)
    accept
        .warning(contribution,
                 "SICI title code must contain at most six characters.")
        .property<&ast::Contribution::title>();
}

void register_validation_checks(SiciCoreServices& services) {
  using V = SiciValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkIssnChecksum>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkControl>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkContribution>(
           validator)});
}

}  // namespace sici
