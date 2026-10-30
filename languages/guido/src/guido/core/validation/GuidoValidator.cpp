#include "guido/core/validation/GuidoValidator.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "guido/core/CoreServices.hpp"
#include <algorithm>
#include <array>
#include <memory>
namespace guido {
void GuidoValidator::checkNote(const ast::Note &node,
                               cursiv::ValidationAcceptor &accept) const {
  if (node.dots.size() > 3)
    accept
        .error(node, "Expected at most three duration dots.",
               "guido.duration-dots")
        .property<&ast::Note::dots>();
  if (node.name == "_" && (node.octave || !node.accidentals.empty()))
    accept.error(node, "A rest has no octave or accidentals.",
                 "guido.rest-pitch");
}
void GuidoValidator::checkDuration(const ast::Duration &node,
                                   cursiv::ValidationAcceptor &accept) const {
  if (node.asterisk && !node.numerator)
    accept.error(node, "Expected a duration numerator.",
                 "guido.duration-numerator");
  for (const auto *n : {node.numerator, node.denominator})
    if (n && (n->negative || n->positive))
      accept.error(*n, "A duration uses unsigned integers.",
                   "guido.duration-sign");
  if (node.denominator) {
    if (node.denominator->value.find_first_not_of('0') ==
        std::string_view::npos)
      accept
          .warning(node, "A duration denominator must be positive.",
                   "guido.duration-denominator")
          .property<&ast::Duration::denominator>();
  }
}
void GuidoValidator::checkTablature(const ast::Tablature &node,
                                   cursiv::ValidationAcceptor &accept) const {
  if (node.dots.size() > 3)
    accept.error(node, "Expected at most three duration dots.",
                 "guido.duration-dots").property<&ast::Tablature::dots>();
}
void GuidoValidator::checkParameter(const ast::Parameter &node,
                                    cursiv::ValidationAcceptor &accept) const {
  constexpr std::array<std::string_view, 8> units = {"m",  "cm", "mm", "in",
                                                     "pt", "pc", "hs", "rl"};
  if (!node.unit.empty() &&
      std::find(units.begin(), units.end(), node.unit) == units.end())
    accept.error(node, "Unknown parameter unit.", "guido.parameter-unit")
        .property<&ast::Parameter::unit>();
}
void GuidoValidator::checkChordMember(
    const ast::ChordMember &node, cursiv::ValidationAcceptor &accept) const {
  const auto count = [&](auto &&self,
                         const ast::SequenceItem *item) -> std::size_t {
    if (cursiv::ast_cast<ast::Note>(item) ||
        cursiv::ast_cast<ast::Tablature>(item))
      return 1;
    if (const auto *tag = cursiv::ast_cast<ast::Tag>(item)) {
      std::size_t total = 0;
      for (const auto *child : tag->notes)
        total += self(self, child);
      return total;
    }
    return 0;
  };
  std::size_t notes = 0;
  for (const auto *item : node.items)
    notes += count(count, item);
  if (notes != 1)
    accept.error(node, "Expected one note or rest per chord member.",
                 "guido.chord-member");
}
void register_validation_checks(GuidoCoreServices &services) {
  services.validator = std::make_unique<const GuidoValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &GuidoValidator::checkNote>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &GuidoValidator::checkTablature>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &GuidoValidator::checkDuration>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &GuidoValidator::checkParameter>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &GuidoValidator::checkChordMember>(*services.validator)});
}
} // namespace guido
