#include "csharp/core/validation/CSharpValidator.hpp"

#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "csharp/core/CoreServices.hpp"

namespace csharp {

void CSharpValidator::checkRawLiteral(const ast::LiteralExpr& literal,
                                      cursiv::ValidationAcceptor& accept) const {
  auto text = literal.text;
  if (!text.starts_with("\"\"\"")) return;
  if (text.ends_with("u8") || text.ends_with("U8")) text.remove_suffix(2);
  const auto first = text.find_first_not_of('"');
  const auto last = text.find_last_not_of('"');
  if (first == std::string_view::npos || last < first ||
      text.size() - last - 1 != first ||
      text.substr(first, last - first + 1).find(std::string(first, '"')) !=
          std::string_view::npos)
    accept.error(literal, "Raw string delimiters must be equal maximal quote runs.",
                 "csharp.raw-delimiter").property<&ast::LiteralExpr::text>();
}

void register_validation_checks(CSharpCoreServices& services) {
  services.validator = std::make_unique<const CSharpValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&CSharpValidator::checkRawLiteral>(
          *services.validator)});
}

}  // namespace csharp
