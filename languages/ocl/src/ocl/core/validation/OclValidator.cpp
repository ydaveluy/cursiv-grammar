#include "ocl/core/validation/OclValidator.hpp"

#include <memory>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "ocl/core/CoreServices.hpp"

namespace ocl {
namespace {
template <class Part>
void checkNames(const auto& parts, cursiv::ValidationAcceptor& accept) {
  std::unordered_set<std::string_view> names;
  for (const Part* part : parts)
    if (part && !names.insert(part->name).second)
      accept.error(*part, "Tuple part names must be unique.", "ocl.tuple-duplicate")
          .template property<&Part::name>();
}
} // namespace

void OclValidator::checkTupleLiteral(
    const ast::TupleLiteral& node, cursiv::ValidationAcceptor& accept) const {
  checkNames<ast::TuplePart>(node.parts, accept);
}
void OclValidator::checkTupleType(
    const ast::TupleType& node, cursiv::ValidationAcceptor& accept) const {
  checkNames<ast::IdentOptType>(node.parts, accept);
}

void register_validation_checks(OclCoreServices& services) {
  services.validator = std::make_unique<const OclValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&OclValidator::checkTupleLiteral>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&OclValidator::checkTupleType>(*services.validator)});
}

}  // namespace ocl
