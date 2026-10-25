#include "gedcom/core/validation/GedcomValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "gedcom/core/CoreServices.hpp"

namespace gedcom {

namespace {
std::string_view normalized(std::string_view digits) {
  const auto first = digits.find_first_not_of('0');
  return first == digits.npos ? std::string_view("0") : digits.substr(first);
}

std::string increment(std::string_view digits) {
  std::string result(normalized(digits));
  for (auto i = result.size(); i > 0; --i) {
    if (result[i - 1] != '9') {
      ++result[i - 1];
      return result;
    }
    result[i - 1] = '0';
  }
  return "1" + result;
}

bool greater(std::string_view left, std::string_view right) {
  return left.size() != right.size() ? left.size() > right.size()
                                     : left > right;
}

bool continuation(const ast::Line &line) {
  return line.tag == "CONT" || line.tag == "CONC";
}
} // namespace

void GedcomValidator::checkLevelIncrement(
    const ast::Document &document, cursiv::ValidationAcceptor &accept) const {
  const ast::Line *previous = nullptr;
  for (const auto *line : document.lines) {
    if (!line)
      continue;
    const auto current = normalized(line->level);
    const std::string limit = previous ? increment(previous->level) : "0";
    if (greater(current, limit))
      accept
          .error(*line, "A level can increase by at most one.",
                 "gedcom.level-jump")
          .property<&ast::Line::level>();
    if (previous && continuation(*line)) {
      const std::string_view expected =
          continuation(*previous) ? normalized(previous->level) : limit;
      if (current != expected)
        accept
            .error(*line,
                   "CONT and CONC must continue their preceding parent record.",
                   "gedcom.continuation-level")
            .property<&ast::Line::level>();
    }
    previous = line;
  }
}

void GedcomValidator::checkUniqueXrefIds(
    const ast::Document &document, cursiv::ValidationAcceptor &accept) const {
  std::unordered_map<std::string_view, std::vector<const ast::Line *>> byName;
  for (const ast::Line *const line : document.lines)
    if (line != nullptr && !line->name.empty())
      byName[line->name].push_back(line);

  for (const auto &[name, lines] : byName) {
    if (lines.size() <= 1)
      continue;
    const std::string message =
        "Duplicate xref_id: @" + std::string(name) + "@";
    for (const ast::Line *const line : lines)
      accept.error(*line, message, "gedcom.duplicate-xref")
          .property<&ast::Line::name>();
  }
}

void register_validation_checks(GedcomCoreServices &services) {
  using V = GedcomValidator;
  services.validator = std::make_unique<const V>(services);
  const V &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkLevelIncrement>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkUniqueXrefIds>(
           validator)});
}

} // namespace gedcom
