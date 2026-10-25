#include "semver/core/validation/SemverValidator.hpp"

#include <algorithm>
#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "semver/core/CoreServices.hpp"

namespace semver {

void SemverValidator::checkVersion(const ast::Semver& node, cursiv::ValidationAcceptor& accept) const {
  auto leadingZero = [](std::string_view t) { return t.size() > 1 && t[0] == '0'; };
  if (node.core) {
    auto check = [&]<auto Member>() {
      if (leadingZero(node.core->*Member))
        accept.error(*node.core, "A numeric version component must not contain leading zeroes.",
                     "semver.core-leading-zero").property<Member>();
    };
    check.template operator()<&ast::VersionCore::major>();
    check.template operator()<&ast::VersionCore::minor>();
    check.template operator()<&ast::VersionCore::patch>();
  }
  if (node.preRelease) for (std::size_t i = 0; i < node.preRelease->identifiers.size(); ++i) {
    const auto t = node.preRelease->identifiers[i];
    if (leadingZero(t) && std::ranges::all_of(t, [](char c) { return c >= '0' && c <= '9'; }))
      accept.error(*node.preRelease, "A numeric prerelease identifier must not contain leading zeroes.",
                   "semver.prerelease-leading-zero").property<&ast::Segment::identifiers>(i);
  }
}

void register_validation_checks(SemverCoreServices& services) {
  services.validator = std::make_unique<const SemverValidator>(services);
  services.validationRegistry->registerChecks({cursiv::ValidationRegistry::makeValidationCheck<&SemverValidator::checkVersion>(*services.validator)});
}

}  // namespace semver
