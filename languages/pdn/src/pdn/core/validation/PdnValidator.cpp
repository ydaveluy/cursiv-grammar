#include "pdn/core/validation/PdnValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "pdn/core/CoreServices.hpp"

namespace pdn {

void PdnValidator::checkUniqueTags(const ast::Game &game,
                                   cursiv::ValidationAcceptor &accept) const {
  std::unordered_map<std::string_view, std::vector<const ast::Tag *>> names;
  for (const ast::Tag *const tag : game.tags)
    if (tag != nullptr)
      names[tag->name].push_back(tag);

  for (const auto &[name, tags] : names) {
    if (tags.size() <= 1)
      continue;
    const std::string message = "Duplicate tag: " + std::string(name);
    for (const ast::Tag *const tag : tags)
      accept.error(*tag, message).property<&ast::Tag::name>();
  }
}

void PdnValidator::checkDocument(const ast::Document &document,
                                 cursiv::ValidationAcceptor &accept) const {
  if (document.games.empty())
    accept.error(document, "A PDN file needs a game.", "pdn.empty");
}

void PdnValidator::checkVariation(const ast::Variation &variation,
                                  cursiv::ValidationAcceptor &accept) const {
  if (variation.items.empty())
    accept.error(variation, "A variation needs a body.", "pdn.variation");
}

void PdnValidator::checkResults(const ast::Game &game,
                                cursiv::ValidationAcceptor &accept) const {
  for (const auto outcome : game.results)
    if (outcome != game.results[0]) {
      accept.error(game, "Conflicting game result markers.", "pdn.results")
          .property<&ast::Game::results>();
      break;
    }
}

void register_validation_checks(PdnCoreServices &services) {
  using V = PdnValidator;
  services.validator = std::make_unique<const V>(services);
  const V &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkUniqueTags>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDocument>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkVariation>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkResults>(
           validator)});
}

} // namespace pdn
