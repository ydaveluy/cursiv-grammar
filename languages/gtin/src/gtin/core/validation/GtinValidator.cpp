#include "gtin/core/validation/GtinValidator.hpp"

#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "gtin/core/CoreServices.hpp"

namespace gtin {

namespace {
bool checkDigit(std::string_view text) {
  unsigned sum = 0, weight = 3;
  for (std::size_t i = text.size() - 1; i > 0; --i) {
    sum += static_cast<unsigned>(text[i - 1] - '0') * weight;
    weight = 4 - weight;
  }
  return (10 - sum % 10) % 10 == static_cast<unsigned>(text.back() - '0');
}

bool upce(std::string_view text, bool withCheck) {
  if (withCheck) {
    if (text.size() != 7 && text.size() != 8) return false;
  } else if (text.size() != 6 && text.size() != 7) {
    return false;
  }
  const char check = withCheck ? text.back() : '\0';
  if (withCheck) text.remove_suffix(1);
  char numberSystem = '0';
  if (text.size() == 7) {
    numberSystem = text.front();
    text.remove_prefix(1);
  }
  if (numberSystem != '0' && numberSystem != '1') return false;
  std::string expanded(12, '0');
  expanded[0] = numberSystem;
  expanded[1] = text[0];
  expanded[2] = text[1];
  const char mode = text[5];
  if (mode <= '2') {
    expanded[3] = mode;
    expanded[8] = text[2];
    expanded[9] = text[3];
    expanded[10] = text[4];
  } else if (mode == '3') {
    if (text[2] <= '2') return false;
    expanded[3] = text[2];
    expanded[9] = text[3];
    expanded[10] = text[4];
  } else if (mode == '4') {
    if (text[3] == '0') return false;
    expanded[3] = text[2];
    expanded[4] = text[3];
    expanded[10] = text[4];
  } else {
    if (text[4] == '0') return false;
    expanded[3] = text[2];
    expanded[4] = text[3];
    expanded[5] = text[4];
    expanded[10] = mode;
  }
  expanded.back() = check;
  return !withCheck || checkDigit(expanded);
}

bool primary(std::string_view text) {
  switch (text.size()) {
    case 6:
    case 7:
      return upce(text, false) || upce(text, true);
    case 8:
      return checkDigit(text) || upce(text, true);
    case 12:
    case 13:
    case 14:
      return checkDigit(text);
    default:
      return false;
  }
}
}  // namespace

void GtinValidator::checkCode(const ast::Gtin& code,
                              cursiv::ValidationAcceptor& accept) const {
  std::string text;
  std::vector<std::size_t> gaps;
  for (const char c : code.text) {
    if (c == '-' || c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
        c == '\f' || c == '\v') {
      if (c != '-' && !text.empty()) gaps.push_back(text.size());
      continue;
    }
    if ((c < '0' || c > '9') && c != '+') {
      accept
          .error(code,
                 "A barcode contains only digits, separators and an optional "
                 "'+' supplement.")
          .property<&ast::Gtin::text>()
          .code("gtin.character");
      return;
    }
    text += c;
  }
  const auto plus = text.find('+');
  if (plus != std::string::npos) {
    const auto supplement = std::string_view(text).substr(plus + 1);
    if ((supplement.size() != 2 && supplement.size() != 5) ||
        supplement.find('+') != std::string_view::npos) {
      accept.error(code, "An EAN supplement must contain 2 or 5 digits.")
          .property<&ast::Gtin::text>()
          .code("gtin.supplement");
      return;
    }
    text.resize(plus);
  } else if (text.size() == 2 || text.size() == 5) {
    return;
  } else if ((text.size() == 15 || text.size() == 18) &&
             primary(std::string_view(text).substr(0, 13))) {
    return;
  }
  if (primary(text)) return;
  if (plus == std::string::npos) {
    for (const auto gap : gaps)
      if ((text.size() - gap == 2 || text.size() - gap == 5) &&
          primary(std::string_view(text).substr(0, gap)))
        return;
  }
  accept
      .error(code,
             "Invalid GTIN/UPC length, check digit or UPC-E zero suppression.")
      .property<&ast::Gtin::text>()
      .code("gtin.code");
}

void register_validation_checks(GtinCoreServices& services) {
  using V = GtinValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkCode>(
          *services.validator)});
}

}  // namespace gtin
