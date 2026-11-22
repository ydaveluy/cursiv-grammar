#include "bencoding/validation/BencodingValidator.hpp"

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "bencoding/CoreServices.hpp"
#include "cursiv/validation/ValidationRegistry.hpp"

namespace bencoding {

void BencodingValidator::checkUniqueKeys(
    const ast::Dict &dict, cursiv::ValidationAcceptor &accept) const {
  std::map<std::string_view, const ast::Entry *> first;
  std::string_view previous;
  for (const ast::Entry *entry : dict.entries) {
    if (!entry || !entry->key) continue;
    const auto key = entry->key->value();
    if (previous > key)
      accept.error(*entry, "Dictionary keys must be in byte order.", "bencoding.key-order")
          .property<&ast::Entry::key>();
    previous = key;
    // Ordered keys insert at the end. Report the first occurrence once.
    const auto size = first.size();
    const auto it = first.try_emplace(first.end(), key, entry);
    if (first.size() == size) {
      const auto duplicate = [&](const ast::Entry &at) {
        accept.error(at, "Duplicate key: " + std::string(key), "bencoding.duplicate-key")
            .property<&ast::Entry::key>();
      };
      if (it->second) duplicate(*std::exchange(it->second, nullptr));
      duplicate(*entry);
    }
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
  const auto length = string.length();
  if (length.size() > 1 && length.front() == '0') {
    const auto begin = accept.spanOf(string).begin;
    accept.error(string, "A byte count cannot have leading zeros.", "bencoding.string-length")
        .range({begin, begin + static_cast<cursiv::TextOffset>(length.size())});
  }
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
