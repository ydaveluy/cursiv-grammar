// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "lambda/CoreServices.hpp"
#include "lambda/validation/LambdaValidator.hpp"
#include "lambda/generated/Lambda/Module.hpp"

namespace lambda {

void install_core_module(LambdaCoreServices& services) {
  lambda::generated::Lambda::installGeneratedModule(services);
  register_validation_checks(services);
}

}  // namespace lambda
