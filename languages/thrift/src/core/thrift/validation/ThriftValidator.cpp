#include "thrift/validation/ThriftValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "thrift/CoreServices.hpp"

namespace thrift {

void register_validation_checks(ThriftCoreServices& services) {
  services.validator = std::make_unique<const ThriftValidator>(services);
}

}  // namespace thrift
