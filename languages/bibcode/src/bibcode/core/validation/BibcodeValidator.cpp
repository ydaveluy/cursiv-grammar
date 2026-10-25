#include "bibcode/core/validation/BibcodeValidator.hpp"

#include <memory>
#include <algorithm>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "bibcode/core/CoreServices.hpp"

namespace bibcode {

void BibcodeValidator::checkFields(const ast::Bibcode& node,
                                   cursiv::ValidationAcceptor& accept) const {
  if (!std::all_of(node.year.begin(), node.year.end(), [](char c) { return c >= '0' && c <= '9'; }))
    accept.error(node, "Year must contain four decimal digits.", "bibcode.year")
        .property<&ast::Bibcode::year>();
  const auto field = [&]<auto Member>() {
    const auto text = node.*Member;
    if (std::any_of(text.begin(), text.end(), [](unsigned char c) { return c <= 32 || c >= 127; }))
      accept.error(node, "A bibcode field cannot contain whitespace or non-ASCII characters.", "bibcode.field")
          .template property<Member>();
  };
  field.operator()<&ast::Bibcode::publish>();
  field.operator()<&ast::Bibcode::volume>();
  field.operator()<&ast::Bibcode::section>();
  field.operator()<&ast::Bibcode::page>();
  field.operator()<&ast::Bibcode::author>();
}

void register_validation_checks(BibcodeCoreServices& services) {
  services.validator = std::make_unique<const BibcodeValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&BibcodeValidator::checkFields>(*services.validator)});
}

}  // namespace bibcode
