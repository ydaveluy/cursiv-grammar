#include "thrift/core/validation/ThriftValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "thrift/core/CoreServices.hpp"

namespace thrift {

void register_validation_checks(ThriftCoreServices& services) {
  services.validator = std::make_unique<const ThriftValidator>(services);
}

}  // namespace thrift
