#include "javacc/core/validation/JavaccValidator.hpp"

#include <memory>
#include <limits>
#include <optional>
#include <string>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "javacc/core/CoreServices.hpp"

namespace javacc {
namespace {
// The native descriptor is one UTF-16 code unit, not one UTF-8 byte.
std::optional<unsigned> unit(std::string_view text) {
  if (text.empty() || text.size() > 3) return {};
  const auto first = static_cast<unsigned char>(text[0]);
  if (first < 0x80) return text.size() == 1 ? std::optional<unsigned>(first) : std::nullopt;
  if (first < 0xc2 || first > 0xef) return {};
  const auto size = first < 0xe0 ? 2u : 3u;
  if (text.size() != size) return {};
  unsigned value = first & (size == 2 ? 31u : 15u);
  for (unsigned i = 1; i < size; ++i) {
    const auto c = static_cast<unsigned char>(text[i]);
    if ((c & 0xc0) != 0x80) return {};
    value = (value << 6) | (c & 63);
  }
  if (size == 3 && value < 0x800) return {};
  return value;
}
constexpr auto maxInt = std::numeric_limits<std::int32_t>::max();
} // namespace

void JavaccValidator::checkOption(const ast::OptionBinding& n, cursiv::ValidationAcceptor& a) const {
  if (const auto* value = std::get_if<std::int64_t>(&n.value); value && *value > maxInt)
    a.error(n, "JavaCC integer options use signed 32-bit values.", "javacc.integer-range");
}
void JavaccValidator::checkLookahead(const ast::LocalLookahead& n, cursiv::ValidationAcceptor& a) const {
  if (n.amount && *n.amount > maxInt)
    a.error(n, "JavaCC lookahead amounts use signed 32-bit values.", "javacc.integer-range");
}
void JavaccValidator::checkRange(const ast::RegexGroupUnit& n, cursiv::ValidationAcceptor& a) const {
  if ((n.repeatMin && *n.repeatMin > maxInt) || (n.repeatMax && *n.repeatMax > maxInt))
    a.error(n, "JavaCC repetition counts use signed 32-bit values.", "javacc.integer-range");
}
void JavaccValidator::checkCharacter(const ast::CharDescriptor& n, cursiv::ValidationAcceptor& a) const {
  const auto from = unit(n.from), to = unit(n.to);
  if (!from || (n.range && !to))
    a.error(n, "A character-list endpoint must contain one UTF-16 code unit.", "javacc.character");
  else if (n.range && *to < *from)
    a.error(n, "The right endpoint is less than the left endpoint.", "javacc.character-order");
}
void JavaccValidator::checkTry(const ast::TryExpansionUnit& n, cursiv::ValidationAcceptor& a) const {
  if (n.catches.empty() && !n.finallyBlock)
    a.error(n, "A try expansion requires a catch or finally block.", "javacc.try-handler");
}

// JavaCC itself refuses when the two names differ: `PARSER_BEGIN(A) ...
// PARSER_END(B)` names two different classes with one parser body.
void JavaccValidator::checkParserNamesMatch(
    const ast::JavaccInput& input, cursiv::ValidationAcceptor& accept) const {
  if (input.parserBeginName != input.parserEndName)
    accept
        .error(input, "PARSER_END(" + std::string(input.parserEndName) +
                          ") does not match PARSER_BEGIN(" +
                          std::string(input.parserBeginName) + ")")
        .property<&ast::JavaccInput::parserEndName>();
}

void register_validation_checks(JavaccCoreServices& services) {
  using V = JavaccValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkParserNamesMatch>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkOption>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkLookahead>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRange>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCharacter>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkTry>(validator)});
}

}  // namespace javacc
