#include "vmf/core/validation/VmfValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "vmf/core/CoreServices.hpp"

namespace vmf {

void VmfValidator::checkKey(const ast::KeyValue& node,
                            cursiv::ValidationAcceptor& accept) const {
  const auto invalidBare = [](std::string_view text) {
    return text.find_first_of("\"'{};,=[]()\r\n\t ") != text.npos ||
           text.starts_with('#') || text.starts_with('/');
  };
  if (!node.keyQuoted && invalidBare(node.key))
    accept.error(node, "This key must be quoted.", "vmf.unquoted-token")
        .property<&ast::KeyValue::key>();
  if (node.value && !node.valueQuoted && invalidBare(*node.value))
    accept.error(node, "This value must be quoted.", "vmf.unquoted-token")
        .property<&ast::KeyValue::value>();
  if (node.key.find_first_of("\r\n") != node.key.npos)
    accept.error(node, "A key cannot contain a newline.", "vmf.key-newline")
        .property<&ast::KeyValue::key>();
}

void register_validation_checks(VmfCoreServices& services) {
  services.validator = std::make_unique<const VmfValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&VmfValidator::checkKey>(
          *services.validator)});
}

}  // namespace vmf
