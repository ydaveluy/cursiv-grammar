#include "nasm/core/validation/NasmValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "nasm/core/CoreServices.hpp"

namespace nasm {

// A colon label (`msg:`) names an address; two lines claiming the same one
// is a real conflict the grammar cannot see, since each `Line` parses on
// its own.
void NasmValidator::checkUniqueLabels(
    const ast::Program& program, cursiv::ValidationAcceptor& accept) const {
  std::unordered_map<std::string, std::vector<const ast::Line*>> labels;
  std::string scope;
  for (const ast::Line* const line : program.lines) {
    if (!line || line->label.empty())
      continue;
    const std::string_view name = line->label;
    const bool local = name.starts_with(".") && !name.starts_with("..");
    const std::string qualified =
        local ? scope + std::string(name) : std::string(name);
    labels[qualified].push_back(line);
    if (!local && !name.starts_with("..@"))
      scope = std::string(name);
  }

  for (const auto& [label, lines] : labels) {
    if (lines.size() <= 1) continue;
    const std::string message = "Duplicate label: " + std::string(label);
    for (const ast::Line* const line : lines)
      accept.error(*line, message).property<&ast::Line::label>();
  }
}

void register_validation_checks(NasmCoreServices& services) {
  using V = NasmValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkUniqueLabels>(validator)});
}

}  // namespace nasm
