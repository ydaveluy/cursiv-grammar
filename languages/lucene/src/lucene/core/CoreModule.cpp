// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "lucene/core/CoreServices.hpp"
#include "lucene/core/validation/LuceneValidator.hpp"
#include "lucene/generated/Lucene/Module.hpp"

namespace lucene {

void install_core_module(LuceneCoreServices& services) {
  lucene::generated::Lucene::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace lucene
