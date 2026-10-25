#include "upnp/core/validation/UpnpValidator.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "upnp/core/CoreServices.hpp"
#include "upnp/core/Value.hpp"
#include <memory>
namespace upnp {
void UpnpValidator::checkComparison(const ast::Comparison &c,
                                    cursiv::ValidationAcceptor &a) const {
  if (comparison_value(c).find('\0') != std::string::npos)
    a.error(
         c,
         "A search value contains NUL; its complete quoted source is retained.")
        .property<&ast::Comparison::raw>()
        .code("upnp.value-nul");
}
void register_validation_checks(UpnpCoreServices &services) {
  using V = UpnpValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkComparison>(
          *services.validator)});
}
} // namespace upnp
