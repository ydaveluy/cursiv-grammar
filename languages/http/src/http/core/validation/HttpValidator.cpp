#include "http/core/validation/HttpValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "http/core/CoreServices.hpp"

namespace http {

void register_validation_checks(HttpCoreServices& services) {
  services.validator = std::make_unique<const HttpValidator>(services);
}

}  // namespace http
