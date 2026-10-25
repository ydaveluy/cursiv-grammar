#include "doiurl/core/validation/DoiurlValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "doiurl/core/CoreServices.hpp"

namespace doiurl {

void register_validation_checks(DoiurlCoreServices& services) {
  services.validator = std::make_unique<const DoiurlValidator>(services);
}

}  // namespace doiurl
