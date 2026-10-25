#include "maml/core/validation/MamlValidator.hpp"

#include <memory>

#include "maml/core/CoreServices.hpp"

namespace maml {

void register_validation_checks(MamlCoreServices& services) {
  services.validator = std::make_unique<const MamlValidator>(services);
}

}  // namespace maml
