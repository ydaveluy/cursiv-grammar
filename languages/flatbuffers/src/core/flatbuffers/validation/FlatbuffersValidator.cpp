#include "flatbuffers/validation/FlatbuffersValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "flatbuffers/CoreServices.hpp"

namespace flatbuffers {

void register_validation_checks(FlatbuffersCoreServices& services) {
  using V = FlatbuffersValidator;
  services.validator = std::make_unique<const V>(services);
}

}  // namespace flatbuffers
