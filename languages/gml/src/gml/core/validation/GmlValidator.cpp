#include "gml/core/validation/GmlValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "gml/core/CoreServices.hpp"

namespace gml {

void GmlValidator::checkEntry(
    const ast::Kv& entry, cursiv::ValidationAcceptor& accept) const {
  const auto letter = [](char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
  };
  bool keyValid = !entry.key.empty() && letter(entry.key.front());
  for (char c : entry.key)
    keyValid &= letter(c) || (c >= '0' && c <= '9') || c == '_';
  if (!keyValid)
    accept.error(entry, "A key must start with an ASCII letter.", "gml.key")
        .property<&ast::Kv::key>();
  if (!entry.quoted && std::holds_alternative<std::string_view>(entry.value) &&
      entry.key != "id" && entry.key != "label" &&
      entry.key != "source" && entry.key != "target")
    accept.error(entry, "Quote a text value.", "gml.value")
        .property<&ast::Kv::value>();
}

void register_validation_checks(GmlCoreServices& services) {
  services.validator = std::make_unique<const GmlValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&GmlValidator::checkEntry>(
          *services.validator)});
}

}  // namespace gml
