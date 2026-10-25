#include "sysmlv2/core/validation/SysmlV2Validator.hpp"

#include <memory>

#include "sysmlv2/core/CoreServices.hpp"

namespace sysmlv2 {

void register_validation_checks(SysmlV2CoreServices& services) {
  services.validator = std::make_unique<const SysmlV2Validator>(services);
}

}  // namespace sysmlv2
