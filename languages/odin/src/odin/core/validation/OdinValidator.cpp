#include "odin/core/validation/OdinValidator.hpp"

#include <memory>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "odin/core/CoreServices.hpp"

namespace odin {

void OdinValidator::checkStructTags(const ast::StructType& node,
                                    cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string_view> seen;
  for (const auto tag : node.tags)
    if (!seen.insert(tag).second) accept.error(node, "Duplicate struct tag.");
}

void register_validation_checks(OdinCoreServices& services) {
  services.validator = std::make_unique<const OdinValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &OdinValidator::checkStructTags>(*services.validator)});
}

}  // namespace odin
