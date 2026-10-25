#include "inf/core/validation/InfValidator.hpp"

#include <memory>

#include "cursiv/core/utils/Utf8.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "inf/core/CoreServices.hpp"

namespace inf {

void InfValidator::checkDocument(const ast::Document& document,
                                 cursiv::ValidationAcceptor& accept) const {
  for (const auto* line : document.preamble)
    accept.error(*line, "An INF data line must belong to a section.",
                 "inf.section");
}

void InfValidator::checkSection(const ast::Section& section,
                                cursiv::ValidationAcceptor& accept) const {
  std::size_t units = 0;
  for (std::size_t i = 0; i < section.name.size();) {
    const auto width = cursiv::utf8_width(section.name[i]);
    units += width == 4 ? 2 : 1;
    i += width;
  }
  if (units > 255)
    accept
        .error(section,
               "An INF section name may contain at most 255 UTF-16 code units.",
               "inf.section-length")
        .property<&ast::Section::name>();
}

void register_validation_checks(InfCoreServices& services) {
  services.validator = std::make_unique<const InfValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &InfValidator::checkDocument>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &InfValidator::checkSection>(*services.validator)});
}

}  // namespace inf
