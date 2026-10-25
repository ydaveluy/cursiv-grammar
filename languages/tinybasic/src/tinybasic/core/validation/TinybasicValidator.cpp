#include "tinybasic/core/validation/TinybasicValidator.hpp"

#include <memory>

#include "tinybasic/core/CoreServices.hpp"

namespace tinybasic {

void register_validation_checks(TinybasicCoreServices& services) {
  services.validator = std::make_unique<const TinybasicValidator>(services);
}

}  // namespace tinybasic
