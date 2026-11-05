#include "http/validation/HttpValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "http/CoreServices.hpp"

namespace http {

void register_validation_checks(HttpCoreServices& services) {
  services.validator = std::make_unique<const HttpValidator>(services);
}

}  // namespace http
