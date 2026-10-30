#include "jpa/core/validation/JpaValidator.hpp"

#include <memory>
#include <string>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "jpa/core/CoreServices.hpp"

namespace jpa {

void JpaValidator::checkDocument(const ast::Document& document,
                                 cursiv::ValidationAcceptor& accept) const {
  bool positional = false, named = false;
  const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
    std::string_view alias;
    if (const auto* n = cursiv::ast_cast<ast::RangeVarDecl>(&node)) alias = n->alias;
    if (const auto* n = cursiv::ast_cast<ast::JoinStep>(&node)) alias = n->alias;
    if (const auto* n = cursiv::ast_cast<ast::CollectionMemberDecl>(&node)) alias = n->alias;
    if (const auto* n = cursiv::ast_cast<ast::AliasedPathDecl>(&node)) alias = n->alias;
    std::string upper(alias);
    for (auto& c : upper) if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    constexpr std::string_view reserved =
        " SELECT FROM WHERE UPDATE DELETE JOIN OUTER INNER LEFT GROUP BY HAVING FETCH DISTINCT "
        "OBJECT NULL TRUE FALSE NOT AND OR BETWEEN LIKE IN AS UNKNOWN EMPTY MEMBER OF IS AVG "
        "MAX MIN SUM COUNT ORDER ASC DESC MOD UPPER LOWER TRIM POSITION CHARACTER_LENGTH "
        "CHAR_LENGTH BIT_LENGTH CURRENT_TIME CURRENT_DATE CURRENT_TIMESTAMP NEW EXISTS ALL ANY SOME ";
    if (!upper.empty() && reserved.find(" " + upper + " ") != std::string_view::npos)
      accept.error(node, "An identification variable cannot be a reserved word.", "jpa.alias-reserved");
    if (const auto* parameter = cursiv::ast_cast<ast::InputParam>(&node)) {
      positional |= parameter->positional;
      named |= !parameter->positional;
      if (parameter->positional &&
          parameter->text.find_first_not_of('0') == std::string_view::npos)
        accept.error(*parameter, "Positional parameters start at 1.", "jpa.parameter-index");
    }
    services.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) { self(self, child); });
  };
  walk(walk, document);
  if (positional && named)
    accept.error(document, "Named and positional parameters cannot be mixed.", "jpa.parameter-style");
}

void register_validation_checks(JpaCoreServices& services) {
  services.validator = std::make_unique<const JpaValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&JpaValidator::checkDocument>(*services.validator)});
}

}  // namespace jpa
