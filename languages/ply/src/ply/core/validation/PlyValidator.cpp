#include "ply/core/validation/PlyValidator.hpp"

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "ply/core/CoreServices.hpp"
#include "ply/core/Data.hpp"
namespace ply {
void PlyValidator::checkDocument(const ast::Document &document,
                                 cursiv::ValidationAcceptor &accept) const {
  const auto data = read_data(document);
  for (const auto &issue : data.issues) {
    auto diagnostic = accept.error(*issue.node, issue.message, issue.code);
    if (issue.raw.data() && document.magic.data()) {
      const auto start = static_cast<cursiv::TextOffset>(issue.raw.data() -
                                                         document.magic.data());
      diagnostic.range(
          {start, static_cast<cursiv::TextOffset>(start + issue.raw.size())});
    }
  }
}
void register_validation_checks(PlyCoreServices &services) {
  services.validator = std::make_unique<const PlyValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &PlyValidator::checkDocument>(*services.validator)});
}
} // namespace ply
