#include "cypher/validation/CypherValidator.hpp"

#include <memory>

#include "cypher/CoreServices.hpp"

namespace cypher {

void register_validation_checks(CypherCoreServices& services) {
  services.validator = std::make_unique<const CypherValidator>(services);
}

}  // namespace cypher
