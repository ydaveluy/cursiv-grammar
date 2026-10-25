#include "muddb/core/validation/MuddbValidator.hpp"
#include "cursiv/core/grammar/Conversion.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "muddb/core/CoreServices.hpp"
#include <memory>
#include <string>
namespace muddb {
namespace {
std::string_view trimmed(std::string_view text) {
  const auto first = text.find_first_not_of(" \t\r\v\f");
  if (first == text.npos)
    return {};
  return text.substr(first, text.find_last_not_of(" \t\r\v\f") - first + 1);
}
} // namespace
void MuddbValidator::checkRoom(const ast::Room &node,
                               cursiv::ValidationAcceptor &accept) const {
  const auto check = [&]<auto Field>(std::string_view name) {
    const auto &text = node.*Field;
    if (text && !trimmed(*text).empty() &&
        !cursiv::default_convert<std::int32_t>(trimmed(*text)))
      accept
          .error(node,
                 "Expected a signed 32-bit integer in " + std::string(name) +
                     ".",
                 "muddb.integer-field")
          .template property<Field>();
  };
  check.template operator()<&ast::Room::location>("location");
  check.template operator()<&ast::Room::contents>("contents");
  check.template operator()<&ast::Room::exits>("exits");
  check.template operator()<&ast::Room::next_room>("next");
  check.template operator()<&ast::Room::owner>("owner");
  check.template operator()<&ast::Room::pennies>("pennies");
  check.template operator()<&ast::Room::flags>("flags");
}
void MuddbValidator::checkDb(const ast::Db &node,
                             cursiv::ValidationAcceptor &accept) const {
  for (std::size_t i = 0; i < node.rooms.size(); ++i) {
    const auto *room = node.rooms[i];
    if (!room)
      continue;
    const auto id =
        cursiv::default_convert<std::uint64_t>(room->roomno.substr(1));
    if (!id || *id.value != i)
      accept
          .error(*room, "Object numbers must start at 0 and increase by one.",
                 "muddb.object-number")
          .property<&ast::Room::roomno>();
  }
}
void MuddbValidator::checkKey(const ast::KeyReference &node,
                              cursiv::ValidationAcceptor &accept) const {
  if (!cursiv::default_convert<std::int32_t>(node.value))
    accept
        .error(node, "Lock reference is outside the object-number range.",
               "muddb.key-reference")
        .property<&ast::KeyReference::value>();
}
void register_validation_checks(MuddbCoreServices &services) {
  services.validator = std::make_unique<const MuddbValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &MuddbValidator::checkRoom>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &MuddbValidator::checkDb>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &MuddbValidator::checkKey>(*services.validator)});
}
} // namespace muddb
