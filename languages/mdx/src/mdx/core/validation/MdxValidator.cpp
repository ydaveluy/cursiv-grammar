#include "mdx/core/validation/MdxValidator.hpp"

#include <memory>
#include <charconv>
#include <array>
#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "mdx/core/CoreServices.hpp"

namespace mdx {

// Two axes named the same (`... ON COLUMNS, ... ON COLUMNS`) is legal by
// this grammar but never a valid query: the axis position is what a
// provider actually keys on.
void MdxValidator::checkDuplicateAxisNames(
    const ast::SelectStatement& select, cursiv::ValidationAcceptor& accept) const {
  std::array<const ast::AxisSpec*, 128> byIndex{};
  unsigned largest = 0;
  bool any = false;
  bool hasExpression = false;
  for (const auto* axis : select.axes) {
    // The provider evaluates Integer_Expression. Keep local checks on known
    // aliases/ordinals without guessing the value or type of a general AST.
    if (axis->axisExpression) { hasExpression = true; continue; }
    std::string name(axis->axisName);
    for (auto& c : name) if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    unsigned index = 128;
    if (!axis->axisFunction) {
      static constexpr std::array aliases = {"COLUMNS", "ROWS", "PAGES", "SECTIONS", "CHAPTERS"};
      for (unsigned i = 0; i < aliases.size(); ++i) if (name == aliases[i]) index = i;
    }
    if (index == 128) {
      const auto r = std::from_chars(name.data(), name.data() + name.size(), index);
      if (r.ec != std::errc{} || r.ptr != name.data() + name.size()) index = 128;
    }
    if (index >= byIndex.size()) {
      accept.error(*axis, "Invalid axis: expected an axis alias or integer0..127.", "mdx.axis")
          .property<&ast::AxisSpec::axisName>();
      continue;
    }
    if (byIndex[index]) {
      accept.error(*axis, "Duplicate axis position.", "mdx.duplicate-axis").property<&ast::AxisSpec::axisName>();
      accept.error(*byIndex[index], "Duplicate axis position.", "mdx.duplicate-axis").property<&ast::AxisSpec::axisName>();
    }
    byIndex[index] = axis;
    largest = std::max(largest, index);
    any = true;
  }
  if (any && !hasExpression) for (unsigned i = 0; i < largest; ++i) if (!byIndex[i]) {
    accept.error(select, "Query axes cannot skip a lower-numbered axis.", "mdx.axis-gap");
    break;
  }
}

void register_validation_checks(MdxCoreServices& services) {
  using V = MdxValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkDuplicateAxisNames>(validator)});
}

}  // namespace mdx
