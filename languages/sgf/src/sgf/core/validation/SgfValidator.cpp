#include "sgf/core/validation/SgfValidator.hpp"

#include <algorithm>
#include <memory>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "sgf/core/CoreServices.hpp"

namespace sgf {
void SgfValidator::checkProperty(const ast::Property& n,
                                 cursiv::ValidationAcceptor& a) const {
  if (n.id.empty())
    a.error(n, "a property identifier must contain an uppercase letter")
        .property<&ast::Property::id>();
  if (n.values.empty()) {
    a.error(n, "a property requires a bracketed value")
        .property<&ast::Property::values>();
    return;
  }
  static constexpr std::string_view scalar[] = {
      "AN", "AP", "B",  "BL", "BM", "BR", "BT", "C",  "CA", "CP", "DM",
      "DO", "DT", "EV", "FF", "FG", "GB", "GC", "GM", "GN", "GW", "HA",
      "HO", "IT", "KM", "KO", "MN", "N",  "OB", "ON", "OT", "OW", "PB",
      "PC", "PL", "PM", "PW", "RE", "RO", "RU", "SO", "ST", "SZ", "TE",
      "TM", "UC", "US", "V",  "W",  "WL", "WR", "WT"};
  if (n.values.size() != 1 &&
      std::ranges::find(scalar, n.id) != std::end(scalar))
    a.error(n, "this property requires exactly one value")
        .property<&ast::Property::values>();
  if ((n.id == "KO" || n.id == "DO" || n.id == "IT") &&
      std::ranges::any_of(n.values, [](auto v) { return !v.empty(); }))
    a.error(n, "this property requires an empty value")
        .property<&ast::Property::values>();
}
void SgfValidator::checkNode(const ast::Node& n,
                             cursiv::ValidationAcceptor& a) const {
  std::unordered_set<std::string_view> seen;
  for (const auto* p : n.properties)
    if (!seen.insert(p->id).second)
      a.error(*p, "a property may occur only once in a node")
          .property<&ast::Property::id>();
}
void register_validation_checks(SgfCoreServices& services) {
  services.validator = std::make_unique<const SgfValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &SgfValidator::checkProperty>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &SgfValidator::checkNode>(*services.validator)});
}
}  // namespace sgf
