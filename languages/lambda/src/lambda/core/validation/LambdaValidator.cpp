#include "lambda/core/validation/LambdaValidator.hpp"

#include <memory>

#include "lambda/core/CoreServices.hpp"

namespace lambda {

void register_validation_checks(LambdaCoreServices& services) {
  services.validator = std::make_unique<const LambdaValidator>(services);
}

}  // namespace lambda
