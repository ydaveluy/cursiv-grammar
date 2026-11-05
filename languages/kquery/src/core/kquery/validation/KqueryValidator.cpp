#include "kquery/validation/KqueryValidator.hpp"

#include <memory>

#include "kquery/CoreServices.hpp"

namespace kquery {

void register_validation_checks(KqueryCoreServices& services) {
  services.validator = std::make_unique<const KqueryValidator>(services);
}

}  // namespace kquery
