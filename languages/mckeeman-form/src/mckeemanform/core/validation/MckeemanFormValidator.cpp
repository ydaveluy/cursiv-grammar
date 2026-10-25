#include "mckeemanform/core/validation/MckeemanFormValidator.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "mckeemanform/core/CoreServices.hpp"

namespace mckeemanform {

namespace {
std::optional<std::uint32_t> codepoint(std::string_view text) {
  if (text.empty()) return std::nullopt;
  const auto first = static_cast<unsigned char>(text.front());
  if (first >= 0x80) {
    std::uint32_t value = first & (0x7f >> text.size());
    for (std::size_t i = 1; i < text.size(); ++i)
      value = (value << 6) | (static_cast<unsigned char>(text[i]) & 0x3f);
    return value;
  }
  if (text.size() == 1)
    return first >= 0x20 ? std::optional<std::uint32_t>(first) : std::nullopt;
  std::uint32_t value = 0;
  for (const char c : text) {
    const int digit = c >= '0' && c <= '9' ? c - '0'
                    : c >= 'a' && c <= 'f' ? c - 'a' + 10
                    : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
    if (digit < 0) return std::nullopt;
    value = value * 16 + static_cast<unsigned>(digit);
  }
  if (value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
    return std::nullopt;
  return value;
}
}  // namespace

void MckeemanFormValidator::checkRange(
    const ast::Range& range, cursiv::ValidationAcceptor& accept) const {
  const auto from = codepoint(range.from);
  const auto to = range.to ? codepoint(*range.to) : from;
  if (!from || !to)
    accept.error(range, "Expected a Unicode scalar or hexadecimal code point.",
                 "mckeemanform.codepoint");
  else if (*from > *to)
    accept.error(range, "The end of a range precedes its start.",
                 "mckeemanform.range-order");
}

void MckeemanFormValidator::checkAlternative(
    const ast::Alternative& alternative,
    cursiv::ValidationAcceptor& accept) const {
  for (const auto& item : alternative.items)
    if (const auto* text = std::get_if<std::string_view>(&item))
      for (const unsigned char c : *text)
        if (c < 0x20) {
          accept.error(alternative, "A literal string cannot contain control characters.",
                       "mckeemanform.string-control");
          break;
        }
}

void MckeemanFormValidator::checkUniqueRuleNames(
    const ast::Document& document, cursiv::ValidationAcceptor& accept) const {
  std::unordered_map<std::string_view, std::vector<const ast::Production*>>
      names;
  for (const ast::Production* const rule : document.rules)
    if (rule != nullptr) names[rule->name].push_back(rule);

  for (const auto& [name, rules] : names) {
    if (rules.size() <= 1) continue;
    const std::string message = "Duplicate rule name: " + std::string(name);
    for (const ast::Production* const rule : rules)
      accept.error(*rule, message).property<&ast::Production::name>();
  }
}

void register_validation_checks(MckeemanFormCoreServices& services) {
  using V = MckeemanFormValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkUniqueRuleNames>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRange>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAlternative>(validator)});
}

}  // namespace mckeemanform
