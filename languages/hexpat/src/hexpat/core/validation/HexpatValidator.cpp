#include "hexpat/core/validation/HexpatValidator.hpp"

#include <memory>
#include <cstdint>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "hexpat/core/CoreServices.hpp"

namespace hexpat {

void HexpatValidator::checkParameters(const ast::Fn& function,
                                      cursiv::ValidationAcceptor& accept) const {
  bool defaultSeen = false;
  for (std::size_t i = 0; i < function.params.size(); ++i) {
    const auto* parameter = function.params[i];
    if (!parameter) continue;
    if (parameter->isPack) {
      if (parameter->isRef || !parameter->type || !parameter->endian.empty() || !parameter->typeArgs.empty() ||
          !parameter->type->path.empty() ||
          parameter->type->name != "auto")
        accept.error(*parameter, "A parameter pack must use auto.", "hexpat.parameter-pack");
      else if (i + 1 != function.params.size())
        accept.error(*parameter, "A parameter pack must appear last.", "hexpat.parameter-pack");
    } else if (parameter->defaultValue) {
      defaultSeen = true;
    } else if (defaultSeen) {
      accept.error(*parameter, "A parameter after a default needs a default value.",
                   "hexpat.default-order");
    }
  }
}

namespace {
bool validLiteral(std::string_view text, bool character) {
  if (text.size() < 2) return false;
  const auto body = text.substr(1, text.size() - 2);
  if (character && !body.starts_with('\\')) return body.size() == 1;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] != '\\' || ++i == body.size()) continue;
    const char escape = body[i];
    if (escape != 'u' && escape != 'U') continue;
    const std::size_t digits = escape == 'u' ? 4 : 8;
    if (i + digits >= body.size()) return false;
    std::uint32_t codepoint = 0;
    for (std::size_t j = 0; j < digits; ++j) {
      const char c = body[++i];
      codepoint = codepoint * 16 + (c >= '0' && c <= '9' ? c - '0' :
                                  c >= 'a' && c <= 'f' ? c - 'a' + 10 : c - 'A' + 10);
    }
    if (codepoint > 0x10ffff || (codepoint >= 0xd800 && codepoint <= 0xdfff) ||
        (character && codepoint > 0x7f)) return false;
  }
  return true;
}
} // namespace

void HexpatValidator::checkString(const ast::StringLit& literal,
                                  cursiv::ValidationAcceptor& accept) const {
  if (!validLiteral(literal.text, false))
    accept.error(literal, "The escape must encode a Unicode scalar value.", "hexpat.literal");
}
void HexpatValidator::checkCharacter(const ast::CharLit& literal,
                                     cursiv::ValidationAcceptor& accept) const {
  if (!validLiteral(literal.text, true))
    accept.error(literal, "A character literal must encode one byte.", "hexpat.literal");
}

void register_validation_checks(HexpatCoreServices& services) {
  using V = HexpatValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkParameters>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkString>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkCharacter>(*services.validator)});
}

}  // namespace hexpat
