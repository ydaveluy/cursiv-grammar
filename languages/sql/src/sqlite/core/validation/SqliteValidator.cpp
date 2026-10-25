#include "sqlite/core/validation/SqliteValidator.hpp"

#include <memory>
#include <string>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "sqlite/core/CoreServices.hpp"
namespace sqlite {
void SqliteValidator::checkBlob(const ast::BlobLit& n,
                                cursiv::ValidationAcceptor& a) const {
  if (n.value.size() % 2 != 0 ||
      n.value.find_first_not_of("0123456789abcdefABCDEF") !=
          std::string_view::npos)
    a.error(n, "a blob requires an even number of hexadecimal digits")
        .property<&ast::BlobLit::value>();
}
void SqliteValidator::checkBind(const ast::BindParam& n,
                                cursiv::ValidationAcceptor& a) const {
  if (n.text.empty()) return;
  if (n.text[0] == '?') {
    if (n.text.size() > 1 &&
        n.text.find_first_not_of('0', 1) == std::string_view::npos)
      a.error(n, "a numbered parameter must have a positive index")
          .property<&ast::BindParam::text>();
  } else {
    bool name = false;
    for (std::size_t i = 1; i < n.text.size() && n.text[i] != '('; ++i)
      name |= n.text[i] != ':';
    if (!name)
      a.error(n, "a named parameter requires a name")
          .property<&ast::BindParam::text>();
  }
}
void SqliteValidator::checkJoin(const ast::JoinStep& n,
                                cursiv::ValidationAcceptor& a) const {
  bool inner = false, cross = false, outer = false, side = false;
  for (auto modifier : n.modifiers) {
    std::string word(modifier);
    for (auto& c : word)
      if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    inner |= word == "INNER";
    cross |= word == "CROSS";
    outer |= word == "OUTER";
    side |= word == "LEFT" || word == "RIGHT" || word == "FULL";
  }
  if (((inner || cross) && (outer || side)) || (outer && !side))
    a.error(n, "incompatible join modifiers")
        .property<&ast::JoinStep::modifiers>();
  if (n.modifiers.size() > 3)
    a.error(n, "a join accepts at most three modifiers")
        .property<&ast::JoinStep::modifiers>();
}
void register_validation_checks(SqliteCoreServices& services) {
  services.validator = std::make_unique<const SqliteValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &SqliteValidator::checkBlob>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &SqliteValidator::checkBind>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &SqliteValidator::checkJoin>(*services.validator)});
}
}  // namespace sqlite
