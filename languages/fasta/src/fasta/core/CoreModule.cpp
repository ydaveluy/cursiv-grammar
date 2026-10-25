// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "fasta/core/CoreServices.hpp"
#include "fasta/core/validation/FastaValidator.hpp"
#include "fasta/generated/Fasta/Module.hpp"

namespace fasta {

void install_core_module(FastaCoreServices& services) {
  fasta::generated::Fasta::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace fasta
