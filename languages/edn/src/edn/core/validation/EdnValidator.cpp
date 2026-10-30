#include "edn/core/validation/EdnValidator.hpp"

#include <memory>
#include <charconv>
#include <optional>
#include <string>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "edn/core/CoreServices.hpp"

namespace edn {

namespace {
// Tags may invoke user readers, and collections require recursive EDN
// equivalence. Keep this check conservative for the scalar keys we know.
std::optional<std::string> scalarKey(const ast::Val& value,
                                    std::optional<std::string_view> ns = {}) {
  if (std::holds_alternative<std::monostate>(value)) return "nil";
  if (const auto* b = std::get_if<bool>(&value)) return *b ? "true" : "false";
  if (const auto* s = std::get_if<std::string_view>(&value)) return "s:" + std::string(*s);
  const auto* ptr = std::get_if<ast::Value*>(&value);
  if (!ptr || !*ptr) return {};
  if (const auto* symbol = cursiv::ast_cast<ast::Symbol>(*ptr)) {
    auto name = std::string(symbol->name);
    if (ns) {
      if (name.starts_with("_/")) name.erase(0, 2);
      else if (name.find('/') == std::string::npos) name = std::string(*ns) + "/" + name;
    }
    return "y:" + name;
  }
  if (const auto* keyword = cursiv::ast_cast<ast::Keyword>(*ptr)) {
    auto name = std::string(keyword->name);
    if (ns) {
      if (name.starts_with("_/")) name.erase(0, 2);
      else if (name.find('/') == std::string::npos) name = std::string(*ns) + "/" + name;
    }
    return "k:" + name;
  }
  if (const auto* character = cursiv::ast_cast<ast::Character>(*ptr))
    return "c:" + std::string(character->text);
  if (const auto* number = cursiv::ast_cast<ast::Number>(*ptr)) {
    auto text = number->text;
    if (text.starts_with("##")) return {};
    if (!text.empty() && text.back() == 'N') text.remove_suffix(1);
    if (!text.empty() && text.front() == '+') text.remove_prefix(1);
    return "n:" + std::string(text);
  }
  return {};
}
}  // namespace

void EdnValidator::checkCharacter(const ast::Character& node,
                                 cursiv::ValidationAcceptor& accept) const {
  const auto text = node.text;
  unsigned cp = 0;
  if (text.size() > 1 && (text[0] == 'o' || text[0] == 'u')) {
    const auto digits = text.substr(1);
    const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), cp,
                                             text[0] == 'o' ? 8 : 16);
    if (error != std::errc{} || end != digits.data() + digits.size() ||
        (text[0] == 'o' && cp > 0377) || (cp >= 0xd800 && cp <= 0xdfff))
      accept.error(node, "Invalid character constant.", "edn.character")
          .property<&ast::Character::text>();
  }
}

void EdnValidator::checkMap(const ast::Map& node,
                           cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string> seen;
  for (const auto* entry : node.entries)
    if (entry) if (auto key = scalarKey(entry->key, node.namespace_); key && !seen.insert(*key).second)
      accept.error(*entry, "Duplicate map key.", "edn.duplicate-key")
          .property<&ast::Entry::key>();
}

void EdnValidator::checkSet(const ast::Set& node,
                           cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string> seen;
  for (const auto& value : node.items)
    if (auto key = scalarKey(value); key && !seen.insert(*key).second)
      accept.error(node, "Duplicate set element.", "edn.duplicate-element");
}

void register_validation_checks(EdnCoreServices& services) {
  services.validator = std::make_unique<const EdnValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&EdnValidator::checkCharacter>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&EdnValidator::checkMap>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&EdnValidator::checkSet>(*services.validator)});
}

}  // namespace edn
