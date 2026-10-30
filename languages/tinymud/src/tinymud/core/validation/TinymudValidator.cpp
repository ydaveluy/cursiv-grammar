#include "tinymud/core/validation/TinymudValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "tinymud/core/CoreServices.hpp"

namespace tinymud {

void TinymudValidator::checkSet(const ast::SetCommand& n, cursiv::ValidationAcceptor& a) const {
  for (const std::string_view word : {"LINK_OK", "DARK", "STICKY", "WIZARD", "TINKER", "TEMPLE", "JUNKPILE",
                                    "HAVEN", "ABODE", "UNWANTED", "ROBOT", "BOT", "TABULAR_WHO", "REVERSED_WHO",
                                    "MALE", "FEMALE", "NEUTER", "UNASSIGNED", "BUILDER", "CONSTRUCTOR"}) {
    if (n.flagName.empty() || n.flagName.size() > word.size()) continue;
    bool prefix = true;
    for (std::size_t i = 0; i < n.flagName.size(); ++i) {
      const char c = n.flagName[i];
      prefix &= (c >= 'a' && c <= 'z' ? char(c - 'a' + 'A') : c) == word[i];
    }
    if (prefix) return;
  }
  a.error(n, "Expected a TinyMUD flag name or its prefix.", "tinymud.flag");
}
void TinymudValidator::checkPlayer(const ast::PcreateCommand& n, cursiv::ValidationAcceptor& a) const {
  if (n.password.empty() || n.password.find_first_of(" \t\v\f") != n.password.npos)
    a.error(n, "A new player needs a nonempty password without whitespace.", "tinymud.password");
  if (n.name.find_first_of(" \t\v\f") != n.name.npos)
    a.error(n, "A player name cannot contain whitespace.", "tinymud.player-name");
}

void register_validation_checks(TinymudCoreServices& services) {
  services.validator = std::make_unique<const TinymudValidator>(services);
  using V = TinymudValidator;
  const auto& v = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkSet>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkPlayer>(v)});
}

}  // namespace tinymud
