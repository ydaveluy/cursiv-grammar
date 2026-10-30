#include "lolcode/core/validation/LolcodeValidator.hpp"

#include <memory>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "lolcode/core/CoreServices.hpp"

namespace lolcode {

// The grammar reads two independent LABELs (`IM IN YR x ... IM OUTTA YR y`);
// the language requires them to name the same loop.
void LolcodeValidator::checkLoopLabelsMatch(
    const ast::LoopStmt& loop, cursiv::ValidationAcceptor& accept) const {
  if (loop.label != loop.endLabel)
    accept.error(loop, "Loop closing label must match its opening label.")
        .property<&ast::LoopStmt::endLabel>();
}

void LolcodeValidator::checkSwitchLiterals(
    const ast::SwitchStmt& statement, cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string_view> seen;
  for (const auto* clause : statement.cases) {
    if (!clause)
      continue;
    const auto literal = clause->literal;
    if (literal.empty())
      continue;
    if (!seen.insert(literal).second)
      accept.error(*clause, "Switch literals must be unique.",
                   "lolcode.switch-duplicate")
          .property<&ast::CaseClause::literal>();
    if (literal.size() < 2 || literal.front() != '"')
      continue;
    // Colon quotes the following character. In particular, ::{name} is
    // constant text, while :{name} interpolates a variable.
    for (std::size_t i = 1; i + 1 < literal.size(); ++i) {
      if (literal[i] != ':')
        continue;
      if (literal[i + 1] == '{') {
        accept.error(*clause, "A switch literal cannot interpolate a variable.",
                     "lolcode.switch-interpolation")
            .property<&ast::CaseClause::literal>();
        break;
      }
      ++i;
    }
  }
}

void register_validation_checks(LolcodeCoreServices& services) {
  using V = LolcodeValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkLoopLabelsMatch>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkSwitchLiterals>(validator)});
}

}  // namespace lolcode
