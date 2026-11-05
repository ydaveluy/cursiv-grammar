#include "cql/validation/CqlValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "cql/CoreServices.hpp"

namespace cql {

void register_validation_checks(CqlCoreServices& services) {
  services.validator = std::make_unique<const CqlValidator>(services);
}

}  // namespace cql
