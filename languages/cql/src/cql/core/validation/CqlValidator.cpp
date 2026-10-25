#include "cql/core/validation/CqlValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cql/core/CoreServices.hpp"

namespace cql {

void register_validation_checks(CqlCoreServices& services) {
  services.validator = std::make_unique<const CqlValidator>(services);
}

}  // namespace cql
