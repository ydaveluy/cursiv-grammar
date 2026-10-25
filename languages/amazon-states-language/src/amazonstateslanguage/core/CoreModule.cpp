// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "amazonstateslanguage/core/CoreServices.hpp"
#include "amazonstateslanguage/core/validation/AmazonStatesLanguageValidator.hpp"
#include "amazonstateslanguage/generated/AmazonStatesLanguage/Module.hpp"

namespace amazonstateslanguage {

void install_core_module(AmazonStatesLanguageCoreServices& services) {
  amazonstateslanguage::generated::AmazonStatesLanguage::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace amazonstateslanguage
