#include "bencoding/core/validation/BencodingValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "bencoding/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace bencoding {

void BencodingValidator::checkUniqueKeys(
    const ast::Dict &dict, cursiv::ValidationAcceptor &accept) const {
  std::unordered_map<std::string_view, std::vector<const ast::Entry *>> keys;
  const ast::ByteString *previous = nullptr;
  for (const ast::Entry *const entry : dict.entries)
    if (entry != nullptr && entry->key != nullptr) {
      keys[entry->key->value].push_back(entry);
      if (previous && previous->value > entry->key->value)
        accept.error(*entry, "Dictionary keys must be in byte order.",
                     "bencoding.key-order").property<&ast::Entry::key>();
      previous = entry->key;
    }

  for (const auto &[key, entries] : keys) {
    if (entries.size() <= 1)
      continue;
    const std::string message = "Duplicate key: " + std::string(key);
    for (const ast::Entry *const entry : entries)
      accept.error(*entry, message, "bencoding.duplicate-key")
          .property<&ast::Entry::key>();
  }
}

void BencodingValidator::checkInteger(
    const ast::Integer &integer, cursiv::ValidationAcceptor &accept) const {
  auto digits = integer.text;
  if (digits.starts_with('-'))
    digits.remove_prefix(1);
  if ((digits.size() > 1 && digits[0] == '0') || integer.text == "-0")
    accept
        .error(integer,
               "Bencoded integers cannot have leading zeros or negative zero.",
               "bencoding.integer-format")
        .property<&ast::Integer::text>();
}

void BencodingValidator::checkString(
    const ast::ByteString &string, cursiv::ValidationAcceptor &accept) const {
  if (string.length.size() > 1 && string.length.front() == '0')
    accept.error(string, "A byte count cannot have leading zeros.",
                 "bencoding.string-length").property<&ast::ByteString::length>();
  auto length = string.length;
  while (length.size() > 1 && length.front() == '0')
    length.remove_prefix(1);
  if (length != std::to_string(string.value.size()))
    accept.error(string,
                 "The byte count does not match the captured payload size.",
                 "bencoding.string-size").property<&ast::ByteString::length>();
}

void BencodingValidator::checkDocument(
    const ast::Document &document, cursiv::ValidationAcceptor &accept) const {
  if (document.values.size() != 1)
    accept.error(document, "A bencoded document needs exactly one value.",
                 "bencoding.document-value").property<&ast::Document::values>();
}

void register_validation_checks(BencodingCoreServices &services) {
  using V = BencodingValidator;
  services.validator = std::make_unique<const V>(services);
  const V &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkInteger>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkUniqueKeys>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkString>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDocument>(validator)});
}

} // namespace bencoding
