#include "sexpression/core/validation/SexpressionValidator.hpp"

#include <memory>
#include <string>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "sexpression/core/CoreServices.hpp"

namespace sexpression {

void SexpressionValidator::checkList(
    const ast::List& list, cursiv::ValidationAcceptor& accept) const {
  if (list.opener.empty() || list.closer.empty()) return;
  const char open = list.opener.back();
  const char expected = open == '(' ? ')' : open == '[' ? ']' : '}';
  if (list.closer[0] != expected)
    accept.error(list, "The closing delimiter must match the opening delimiter.",
                 "sexpression.delimiter")
        .property<&ast::List::closer>();

  const auto count = [](const auto& items) {
    std::size_t size = 0;
    for (const auto* item : items)
      if (!cursiv::ast_cast<ast::Discard>(item) &&
          !cursiv::ast_cast<ast::Directive>(item)) ++size;
    return size;
  };
  if (list.opener.starts_with('#')) {
    auto size = list.opener.substr(1, list.opener.size() - 2);
    if (size.size() >= 2 && (size[0] == 'f' || size[0] == 'F') &&
        (size[1] == 'l' || size[1] == 'L' || size[1] == 'x' || size[1] == 'X'))
      size.remove_prefix(2);
    bool explicitSize = !size.empty();
    for (const char c : size) explicitSize &= c >= '0' && c <= '9';
    if (explicitSize) {
      const auto first = size.find_first_not_of('0');
      size = first == std::string_view::npos ? std::string_view("0") : size.substr(first);
      const auto elements = std::to_string(count(list.items));
      if (elements.size() > size.size() ||
          (elements.size() == size.size() && elements > size))
        accept.error(list, "The vector contains more elements than its declared size.",
                     "sexpression.vector-size")
            .property<&ast::List::opener>();
    }
  }
  if (!list.tail) return;
  if (count(list.items) == 0 || (list.infix && count(list.suffix) == 0))
    accept.error(list, "A dot needs a datum on each side.", "sexpression.dot")
        .property<&ast::List::tail>();

  // Vector and hash-table elements form proper lists; prefab structures
  // permit the two-dot infix convention, but not an improper-list tail.
  if (list.opener.starts_with('#') &&
      !(list.infix && list.opener.substr(0, list.opener.size() - 1) == "#s"))
    accept.error(list, "This container does not permit a dotted tail.",
                 "sexpression.container-dot")
        .property<&ast::List::tail>();
}

void register_validation_checks(SexpressionCoreServices& services) {
  using V = SexpressionValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkList>(
          *services.validator)});
}

}  // namespace sexpression
