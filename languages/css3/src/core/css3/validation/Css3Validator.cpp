#include "css3/validation/Css3Validator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "css3/CoreServices.hpp"

namespace css3 {

void register_validation_checks(Css3CoreServices& services) {
  services.validator = std::make_unique<const Css3Validator>(services);
}

}  // namespace css3
