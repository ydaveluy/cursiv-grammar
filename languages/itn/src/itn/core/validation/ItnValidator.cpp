#include "itn/core/validation/ItnValidator.hpp"

#include <memory>

#include "itn/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace itn {

void ItnValidator::checkLine(const ast::Line& line, cursiv::ValidationAcceptor& accept) const {
  if (line.longitude < -18000000 || line.longitude > 18000000)
    accept.error(line, "Longitude must be between -18000000 and 18000000.", "itn.longitude").property<&ast::Line::longitude>();
  if (line.latitude < -9000000 || line.latitude > 9000000)
    accept.error(line, "Latitude must be between -9000000 and 9000000.", "itn.latitude").property<&ast::Line::latitude>();
  if (line.flag.size() != 1)
    accept.error(line, "An ITN flag is one digit.", "itn.flag").property<&ast::Line::flag>();
}

void register_validation_checks(ItnCoreServices& services) {
  services.validator = std::make_unique<const ItnValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&ItnValidator::checkLine>(*services.validator)});
}

}  // namespace itn
