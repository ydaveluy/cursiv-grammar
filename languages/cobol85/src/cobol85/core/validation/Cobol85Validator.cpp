#include "cobol85/core/validation/Cobol85Validator.hpp"

#include <memory>

#include "cobol85/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace cobol85 {

void Cobol85Validator::checkCommunication(const ast::CommunicationEntry& entry,
                                         cursiv::ValidationAcceptor& accept) const {
  if (entry.records.empty())
    accept.error(entry, "A communication description needs a record.", "cobol85.communication-record");
}

void Cobol85Validator::checkEndName(const ast::ProgramUnit& p,
                                    cursiv::ValidationAcceptor& accept) const {
  if (!p.hasEndProgram) return;
  auto body = [](std::string_view t) {
    return t.size() >= 2 && (t[0] == '\'' || t[0] == '"') ? t.substr(1, t.size() - 2)
                                                          : t;
  };
  auto a = body(p.name), b = body(p.endName);
  bool equal = a.size() == b.size();
  for (std::size_t i = 0; equal && i < a.size(); ++i) {
    auto fold = [](unsigned char c) {
      return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
    };
    equal = fold(a[i]) == fold(b[i]);
  }
  if (!equal)
    accept.error(p, "END PROGRAM name does not match PROGRAM-ID.")
        .property<&ast::ProgramUnit::endName>();
}

void register_validation_checks(Cobol85CoreServices& services) {
  services.validator = std::make_unique<const Cobol85Validator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&Cobol85Validator::checkCommunication>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&Cobol85Validator::checkEndName>(
          *services.validator)});
}

}  // namespace cobol85
