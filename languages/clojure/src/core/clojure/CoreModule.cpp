// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "clojure/CoreServices.hpp"
#include "clojure/validation/ClojureValidator.hpp"
#include "clojure/generated/Clojure/Module.hpp"

namespace clojure {

void install_core_module(ClojureCoreServices& services) {
  clojure::generated::Clojure::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace clojure
