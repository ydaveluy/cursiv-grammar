#include "pgn/core/validation/PgnValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "pgn/core/CoreServices.hpp"
#include "pgn/core/Values.hpp"
namespace pgn {
void PgnValidator::checkGame(const ast::Game& n,
                             cursiv::ValidationAcceptor& a) const {
  for (const auto* t : n.tags)
    if (t->name == "Result") {
      if (t->value != "*" && t->value != "1-0" && t->value != "0-1" &&
          t->value != "1/2-1/2")
        a.error(*t, "unknown game result").property<&ast::TagPair::value>();
      else if (n.terminated && n.result != ast::Result::unknown &&
               t->value != "*" && t->value != result_value(n.result))
        a.error(*t, "the Result tag disagrees with the movetext result")
            .property<&ast::TagPair::value>();
    }
}
void PgnValidator::checkVariation(const ast::Variation& n,
                                  cursiv::ValidationAcceptor& a) const {
  if (!n.closed) a.error(n, "a variation requires a closing parenthesis");
}
void PgnValidator::checkComment(const ast::Comment& n,
                                cursiv::ValidationAcceptor& a) const {
  if (!n.raw.ends_with('}'))
    a.error(n, "a comment requires a closing brace")
        .property<&ast::Comment::raw>();
}
void PgnValidator::checkUnexpectedClose(const ast::UnexpectedClose& n,
                                        cursiv::ValidationAcceptor& a) const {
  a.error(n, "a closing parenthesis has no matching variation");
}
void register_validation_checks(PgnCoreServices& services) {
  services.validator = std::make_unique<const PgnValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &PgnValidator::checkGame>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &PgnValidator::checkVariation>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &PgnValidator::checkComment>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &PgnValidator::checkUnexpectedClose>(*services.validator)});
}
}  // namespace pgn
