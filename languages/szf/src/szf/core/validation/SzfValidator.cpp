#include "szf/core/validation/SzfValidator.hpp"

#include <algorithm>
#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "szf/core/CoreServices.hpp"

namespace szf {
void SzfValidator::checkHeader(const ast::Header& header,
                               cursiv::ValidationAcceptor& accept) const {
  const bool valid = !header.name.empty() &&
                     std::ranges::all_of(header.name, [](unsigned char c) {
                       return (c >= 'a' && c <= 'z') ||
                              (c >= 'A' && c <= 'Z') ||
                              (c >= '0' && c <= '9') || c == '_';
                     });
  if (!valid)
    accept
        .error(header, "A header name must be an ASCII identifier.",
               "sfz.header")
        .property<&ast::Header::name>();
}
void SzfValidator::checkDirective(const ast::UnknownDirective& directive,
                                  cursiv::ValidationAcceptor& accept) const {
  accept.error(directive, "Unknown SFZ directive.", "sfz.directive");
}
void SzfValidator::checkFile(const ast::File& file,
                             cursiv::ValidationAcceptor& accept) const {
  for (const auto* element : file.elements) {
    if (cursiv::ast_cast<ast::Header>(element)) break;
    if (const auto* opcode = cursiv::ast_cast<ast::Opcode>(element))
      accept.warning(*opcode, "An opcode before a header has no effect.",
                     "sfz.orphan-opcode");
  }
}
void register_validation_checks(SzfCoreServices& services) {
  using V = SzfValidator;
  services.validator = std::make_unique<const V>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkHeader>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDirective>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkFile>(
           validator)});
}
}  // namespace szf
