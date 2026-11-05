#include "ttm/validation/TtmValidator.hpp"

#include <memory>

#include "ttm/CoreServices.hpp"

namespace ttm {

void register_validation_checks(TtmCoreServices& services) {
  services.validator = std::make_unique<const TtmValidator>(services);
}

}  // namespace ttm
