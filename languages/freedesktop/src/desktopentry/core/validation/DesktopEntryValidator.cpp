#include "desktopentry/core/validation/DesktopEntryValidator.hpp"

#include <memory>
#include <string>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "desktopentry/core/CoreServices.hpp"

namespace desktopentry {

void DesktopEntryValidator::checkDocument(
    const ast::Document& document, cursiv::ValidationAcceptor& accept) const {
  if (document.groups.empty() || document.groups[0]->name != "Desktop Entry")
    accept.error(document, "The first group must be Desktop Entry.",
                 "desktopentry.first-group");
  std::unordered_set<std::string_view> names;
  for (const auto* group : document.groups)
    if (!names.insert(group->name).second)
      accept.error(*group, "Group names must be unique.",
                   "desktopentry.duplicate-group").property<&ast::Group::name>();
}

void DesktopEntryValidator::checkGroup(
    const ast::Group& group, cursiv::ValidationAcceptor& accept) const {
  for (const unsigned char c : group.name)
    if (c < 0x20 || c >= 0x7f) {
      accept.error(group, "Group names use printable ASCII characters.",
                   "desktopentry.group-name").property<&ast::Group::name>();
      break;
    }
  std::unordered_set<std::string> keys;
  for (const auto* entry : group.entries) {
    std::string key(entry->key);
    if (const auto* locale = entry->locale) {
      key += '[';
      key += locale->language;
      if (!locale->country.empty()) { key += '_'; key += locale->country; }
      if (!locale->encoding.empty()) { key += '.'; key += locale->encoding; }
      if (!locale->modifier.empty()) { key += '@'; key += locale->modifier; }
      key += ']';
    }
    if (!keys.insert(key).second)
      accept.error(*entry, "Keys including their locale must be unique in a group.",
                   "desktopentry.duplicate-key").property<&ast::Entry::key>();
  }
}

void DesktopEntryValidator::checkEntry(
    const ast::Entry& entry, cursiv::ValidationAcceptor& accept) const {
  if (entry.key.empty() ||
      entry.key.find_first_not_of(
          "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-") !=
          entry.key.npos)
    accept.error(entry, "Key names use A-Z, a-z, 0-9 and hyphen.",
                 "desktopentry.key-name").property<&ast::Entry::key>();
}

void register_validation_checks(DesktopEntryCoreServices& services) {
  services.validator =
      std::make_unique<const DesktopEntryValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<
          &DesktopEntryValidator::checkDocument>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<
          &DesktopEntryValidator::checkGroup>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<
          &DesktopEntryValidator::checkEntry>(*services.validator)});
}

}  // namespace desktopentry
