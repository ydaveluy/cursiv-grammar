#include "gql/validation/GqlValidator.hpp"

#include <memory>

#include "gql/CoreServices.hpp"

namespace gql {

void register_validation_checks(GqlCoreServices& services) {
  services.validator = std::make_unique<const GqlValidator>(services);
}

}  // namespace gql
