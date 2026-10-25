#include "rfc1960/core/validation/Rfc1960Validator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "rfc1960/core/CoreServices.hpp"
#include "rfc1960/core/FilterValue.hpp"

namespace rfc1960 {

void Rfc1960Validator::checkSimple(const ast::Simple &node,
                                   cursiv::ValidationAcceptor &accept) const {
  if (!attribute_name(node.attr)) {
    auto diagnostic = accept.error(node, "Invalid LDAP attribute description.",
                                   "rfc1960.attribute");
    if (!node.attr.empty())
      diagnostic.property<&ast::Simple::attr>();
  }
  if (!filter_value(node.value))
    accept
        .error(node,
               "Invalid LDAP assertion-value escape or unescaped special "
               "character.",
               "rfc1960.value")
        .property<&ast::Simple::value>();
}

void Rfc1960Validator::checkSubstring(
    const ast::Substring &node, cursiv::ValidationAcceptor &accept) const {
  if (!attribute_name(node.attr)) {
    auto diagnostic = accept.error(node, "Invalid LDAP attribute description.",
                                   "rfc1960.attribute");
    if (!node.attr.empty())
      diagnostic.property<&ast::Substring::attr>();
  }
  if ((node.initial && !filter_value(*node.initial)) ||
      (node.final_ && !filter_value(*node.final_)))
    accept.error(node, "Invalid LDAP substring escape.", "rfc1960.value");
  for (auto part : node.middle) {
    if (part.empty())
      accept.error(
          node,
          "Consecutive LDAP substring wildcards leave an empty component.",
          "rfc1960.substring");
    else if (!filter_value(part))
      accept.error(node, "Invalid LDAP substring escape.", "rfc1960.value")
          .property<&ast::Substring::middle>();
  }
}

void Rfc1960Validator::checkExtensible(
    const ast::Extensible &node, cursiv::ValidationAcceptor &accept) const {
  if (node.attr && !attribute_name(*node.attr))
    accept
        .error(node, "Invalid LDAP attribute description.", "rfc1960.attribute")
        .property<&ast::Extensible::attr>();
  const bool dn = !node.qualifiers.empty() && dn_qualifier(node.qualifiers[0]);
  const auto rules = node.qualifiers.size() - (dn ? 1 : 0);
  if (rules > 1 || (rules == 1 && !oid(node.qualifiers.back())) ||
      (!node.attr && rules == 0))
    accept
        .error(node,
               "An extensible match needs an attribute or one matching rule, "
               "optionally preceded by dn.",
               "rfc1960.matching-rule")
        .property<&ast::Extensible::qualifiers>();
  if (!filter_value(node.value))
    accept
        .error(node,
               "Invalid LDAP assertion-value escape or unescaped special "
               "character.",
               "rfc1960.value")
        .property<&ast::Extensible::value>();
}

void Rfc1960Validator::checkNot(const ast::Not &node,
                                cursiv::ValidationAcceptor &accept) const {
  if (!node.operand)
    accept.error(node, "LDAP negation needs an operand.", "rfc1960.not");
}

void register_validation_checks(Rfc1960CoreServices &services) {
  services.validator = std::make_unique<const Rfc1960Validator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &Rfc1960Validator::checkSimple>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &Rfc1960Validator::checkSubstring>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &Rfc1960Validator::checkExtensible>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &Rfc1960Validator::checkNot>(*services.validator)});
}

} // namespace rfc1960
