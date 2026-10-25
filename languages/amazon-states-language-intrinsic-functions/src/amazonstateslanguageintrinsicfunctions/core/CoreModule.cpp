// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "amazonstateslanguageintrinsicfunctions/core/CoreServices.hpp"
#include "amazonstateslanguageintrinsicfunctions/core/validation/AmazonStatesLanguageIntrinsicFunctionsValidator.hpp"
#include "amazonstateslanguageintrinsicfunctions/generated/AmazonStatesLanguageIntrinsicFunctions/Module.hpp"

namespace amazonstateslanguageintrinsicfunctions {

void install_core_module(AmazonStatesLanguageIntrinsicFunctionsCoreServices& services) {
  amazonstateslanguageintrinsicfunctions::generated::AmazonStatesLanguageIntrinsicFunctions::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace amazonstateslanguageintrinsicfunctions
