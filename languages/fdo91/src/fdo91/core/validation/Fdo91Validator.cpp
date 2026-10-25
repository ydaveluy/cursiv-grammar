#include "fdo91/core/validation/Fdo91Validator.hpp"

#include <memory>
#include "cursiv/core/grammar/Conversion.hpp"

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "fdo91/core/CoreServices.hpp"

namespace fdo91 {

void Fdo91Validator::checkNumber(const ast::Number& node,
                                cursiv::ValidationAcceptor& accept) const {
  if (!cursiv::default_convert<std::int64_t>(node.text))
    accept.error(node, "Expected a signed 64-bit integer.", "fdo91.number-range")
        .property<&ast::Number::text>();
}

void Fdo91Validator::checkPiped(const ast::Piped& node,
                               cursiv::ValidationAcceptor& accept) const {
  if (!node.parts.empty() && !std::holds_alternative<ast::Ident*>(node.parts[0]))
    accept.error(node, "A piped argument must start with a name.", "fdo91.pipe-first-name")
        .property<&ast::Piped::parts>();
  for (std::size_t i = 1; i < node.parts.size(); ++i)
    if (!std::holds_alternative<ast::Ident*>(node.parts[i]) &&
        !std::holds_alternative<ast::Number*>(node.parts[i]))
      accept.error(node, "Expected a name or an integer after '|'.", "fdo91.pipe-part")
          .property<&ast::Piped::parts>();
}

void register_validation_checks(Fdo91CoreServices& services) {
  services.validator = std::make_unique<const Fdo91Validator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&Fdo91Validator::checkNumber>(
          *services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&Fdo91Validator::checkPiped>(
          *services.validator)});
}

}  // namespace fdo91
