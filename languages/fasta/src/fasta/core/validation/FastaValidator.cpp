#include "fasta/core/validation/FastaValidator.hpp"

#include <memory>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "fasta/core/CoreServices.hpp"

namespace fasta {

void FastaValidator::checkSequence(
    const ast::SequenceLine& line, cursiv::ValidationAcceptor& accept) const {
  if (!cursiv::ast_cast<ast::Record>(line.parent)) return;
  for (unsigned char c : line.text)
    if (c >= 0x80) {
      accept.error(line, "Sequence data must be ASCII.", "fasta.sequence-ascii")
          .property<&ast::SequenceLine::text>();
      break;
    }
}

void register_validation_checks(FastaCoreServices& services) {
  using V = FastaValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkSequence>(validator)});
}

}  // namespace fasta
