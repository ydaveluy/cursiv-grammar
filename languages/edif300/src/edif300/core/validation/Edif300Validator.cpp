#include "edif300/core/validation/Edif300Validator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "edif300/core/CoreServices.hpp"

namespace edif300 {

// No checks registered: the grammar alone decides what a document means.
void register_validation_checks(Edif300CoreServices& services) {
  using V = Edif300Validator;
  services.validator = std::make_unique<const V>(services);
}

}  // namespace edif300
