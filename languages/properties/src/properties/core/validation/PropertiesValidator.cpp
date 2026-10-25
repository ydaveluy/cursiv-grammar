#include "properties/core/validation/PropertiesValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "properties/core/CoreServices.hpp"
#include "properties/core/PropertiesString.hpp"

namespace properties {

void PropertiesValidator::checkEscapes(
    const ast::Entry &entry, cursiv::ValidationAcceptor &accept) const {
  const auto *document = entry.getDocument();
  if (!document)
    return;
  const auto source = document->textDocument().getText();
  for (const auto property : {"key", "value"}) {
    const auto span = accept.featureSpan(entry, property, 0);
    if (!span || span->end < span->begin || span->end > source.size())
      continue;
    const auto raw = source.substr(span->begin, span->end - span->begin);
    if (raw.find("\\u") == std::string_view::npos)
      continue;
    const auto value = decode_text(raw);
    if (value.invalid_unicode == std::string_view::npos)
      continue;
    const auto at =
        static_cast<cursiv::TextOffset>(span->begin + value.invalid_unicode);
    accept.error(cursiv::TextRange{at, at + 2},
                 "A Unicode escape needs four hexadecimal digits.",
                 "properties.unicode");
  }
}

// The last occurrence of a key wins (java.util.Properties semantics); every
// earlier one is dead and worth a warning.
void PropertiesValidator::checkDuplicateKey(
    const ast::Document &document, cursiv::ValidationAcceptor &accept) const {
  std::unordered_map<std::string_view, std::vector<const ast::Entry *>> byKey;
  for (const auto &row : document.rows)
    if (const auto *const *entry = std::get_if<ast::Entry *>(&row);
        entry != nullptr && *entry != nullptr)
      byKey[(*entry)->key].push_back(*entry);

  for (const auto &[key, entries] : byKey) {
    if (entries.size() <= 1)
      continue;
    for (std::size_t i = 0; i + 1 < entries.size(); ++i)
      accept
          .warning(*entries[i], "Duplicate key '" + std::string(key) +
                                    "' -- the last occurrence in the file "
                                    "wins.")
          .property<&ast::Entry::key>();
  }
}

void register_validation_checks(PropertiesCoreServices &services) {
  using V = PropertiesValidator;
  services.validator = std::make_unique<const V>(services);
  const V &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkDuplicateKey>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkEscapes>(
           validator)});
}

} // namespace properties
