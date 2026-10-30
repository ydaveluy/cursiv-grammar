#include "zig/core/validation/ZigValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "zig/core/CoreServices.hpp"
#include "zig/core/Literal.hpp"

namespace zig {

namespace {
std::string_view numberError(std::string_view text) {
  unsigned base = 10;
  std::size_t i = 0;
  if (text.size() > 1 && text[0] == '0') {
    switch (text[1]) {
      case 'b': base = 2; i = 2; break;
      case 'o': base = 8; i = 2; break;
      case 'x': base = 16; i = 2; break;
      case 'B': case 'O': case 'X': return "Use a lowercase base prefix";
      case '.': case 'e': case 'E': break;
      default: return "A decimal number cannot have leading zeroes";
    }
  }
  if (i == text.size()) return "Expected a digit after the base prefix";
  bool underscore = false, period = false, exponent = false;
  char special = 0;
  for (; i < text.size(); ++i) {
    const char c = text[i];
    if (c == '_') {
      if ((i == 2 && base != 10) || special)
        return "A digit separator must follow a digit";
      if (underscore) return "Repeated digit separator";
      underscore = true;
      continue;
    }
    if (((c == 'e' || c == 'E') && base == 10) ||
        ((c == 'p' || c == 'P') && base == 16)) {
      if (i == 2 && base == 16) return "Expected a digit after the base prefix";
      if (exponent) return "A number cannot have two exponents";
      if (underscore) return "A digit separator cannot precede an exponent";
      exponent = true;
      special = c;
      continue;
    }
    if (c == '.') {
      if (exponent) return "A decimal point cannot follow an exponent";
      if (base != 10 && base != 16) return "A float must use base 10 or 16";
      if (period) return "A number cannot have two decimal points";
      if (underscore) return "A digit separator cannot precede a decimal point";
      period = true;
      special = c;
      continue;
    }
    if (c == '+' || c == '-') {
      if (special != 'p' && special != 'P' &&
          !((special == 'e' || special == 'E') && base == 10))
        return "A sign must immediately follow the exponent marker";
      special = c;
      continue;
    }
    const unsigned digit = c >= '0' && c <= '9' ? unsigned(c - '0')
                           : c >= 'a' && c <= 'z' ? unsigned(c - 'a' + 10)
                           : c >= 'A' && c <= 'Z' ? unsigned(c - 'A' + 10) : 36;
    if (digit >= base) return "Invalid digit for the number's base";
    if (exponent && digit >= 10) return "An exponent must use decimal digits";
    underscore = false;
    special = 0;
  }
  if (underscore) return "A number cannot end with a digit separator";
  if (special) return "Expected a digit at the end of the number";
  return {};
}
}  // namespace

void ZigValidator::checkNumber(const ast::NumberLiteral& number,
                               cursiv::ValidationAcceptor& accept) const {
  if (const auto error = numberError(number.text); !error.empty())
    accept.error(number, error, "zig.number").property<&ast::NumberLiteral::text>();
}

void ZigValidator::checkFieldOrder(const ast::ContainerMembers& members,
                                   cursiv::ValidationAcceptor& accept) const {
  bool sawField = false;
  const ast::ContainerMember* declaration = nullptr;
  for (const auto* member : members.members) {
    if (!member) continue;
    if (member->type == &cursiv::ast_type_tag<ast::ContainerField>) {
      if (declaration)
        accept.error(*member, "Container fields must precede declarations after the first field",
                     "zig.field-order").relatedInformation(*declaration, "Declaration between fields");
      sawField = true;
    } else if (sawField && !declaration) {
      declaration = member;
    }
  }
}

void ZigValidator::checkUniqueContainerFields(
    const ast::ContainerMembers& members,
    cursiv::ValidationAcceptor& accept) const {
  std::unordered_map<std::string, std::vector<const ast::ContainerField*>>
      names;
  for (const ast::ContainerMember* const member : members.members) {
    if (member == nullptr ||
        member->type != &cursiv::ast_type_tag<ast::ContainerField>)
      continue;
    const auto& field = *static_cast<const ast::ContainerField*>(member);
    if (field.name) {
      std::string name(*field.name);
      if (name.starts_with("@\"")) {
        auto decoded=decodeLiteral(std::string_view(name).substr(1));
        if (!decoded.error.empty()) continue;
        name=std::move(decoded.value);
      }
      names[std::move(name)].push_back(&field);
    }
  }
  for (const auto& [name, fields] : names) {
    if (fields.size() <= 1) continue;
    const std::string message = "Duplicate field name: " + std::string(name);
    for (const ast::ContainerField* const field : fields)
      accept.error(*field, message, "zig.duplicate-field").property<&ast::ContainerField::name>();
  }
}

void ZigValidator::checkModifier(const ast::TypeModifier& modifier,
                                 cursiv::ValidationAcceptor& accept) const {
  const auto repeated = [&](const auto* type) {
    if (!type) return false;
    for (const auto* previous : type->modifiers) {
      if (previous == &modifier) break;
      if (previous && previous->kind == modifier.kind) return true;
    }
    return false;
  };
  if (repeated(cursiv::ast_cast<ast::PtrType>(modifier.parent)) ||
      repeated(cursiv::ast_cast<ast::SliceType>(modifier.parent)) ||
      repeated(cursiv::ast_cast<ast::ManyPtrType>(modifier.parent)))
    accept.error(modifier,"This type qualifier may occur only once","zig.type-modifier")
        .property<&ast::TypeModifier::kind>();
}

void register_validation_checks(ZigCoreServices& services) {
  using V = ZigValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkUniqueContainerFields>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkFieldOrder>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkNumber>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkModifier>(validator)});
}

}  // namespace zig
